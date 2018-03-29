//
// Created by shgli on 17-9-27.
//

#ifndef ARAGOPROJECT_DEVENTNODE_H
#define ARAGOPROJECT_DEVENTNODE_H
#include <boost/lockfree/spsc_queue.hpp>


#include "common/Types.h"
#include "Worker.h"
#include "types.h"
#include "ITask.h"
#include "Context.h"
//#include "SharedMutex.h"
#include <iostream>

#include <boost/circular_buffer.hpp>


class SharedMutex
{
    struct WaiterType
    {
        WaiterType(ITask* ptr, bool write):pTask(ptr), IsWriter(write) {}
        ITask* pTask;
        bool IsWriter;
    };

public:
    //using WaiterBufferType = boost::circular_buffer<WaiterType>;
    using WaiterBufferType = boost::lockfree::spsc_queue<WaiterType,
                                                        boost::lockfree::capacity<1024*128>, 
                                                        boost::lockfree::fixed_sized<true>>;
    SharedMutex() = default;
    SharedMutex(const SharedMutex&) = delete;
    SharedMutex& operator=(const SharedMutex&) = delete;

    /*
    mReaders 代表了当前有多少个Reader Task处于Runing状态
    mWriter 代表当前flow是否有Writer存在， 
    1. 为了保证线程安全，Dispatcher中push_back; 当前节点运行结束，将所有的ReadWaiter + WriteWaiter in NextFlow 都pop_back
    2. Resume ReadWaiter时，mReaders++, 用来阻止下一轮的Writer运行;所以所有的 Reader运行结束时都要UnlockShared, mReaders--
    mReaders 代表了有多少个Reader存在
    mWriter 只能有一个，代表了锁的owner
    */

//Dispatcher调用
    void SetShared(ITask* ptr)
    {
        //mReaders++; 还没有Reader正在请求资源,所以不用设置
        LOG_ERROR("Task[%p]-%s[%d] is Setshared in waitersqueue", ptr, ptr->GetName().c_str(), ptr->GetWorkflowId());
        assert(mWaiters.push(WaiterType(ptr, false)));
    }

/*Dispatcher中同一个节点的两个不同flow的Task不能同时SetLock */
/*
A[1]                A[2]                    A[3] SetLock()

                 if(!mWriter)
                 {      

                 }
                 mWaiters.push();   
                 return false;
mWriter=false;                              A[2] mWaiters.push完成后mWaiters不为空
                                            if(mWaiters.empty()) { return true;}

                                             如果不对mWaiters.front进行判断        lock成功
                                              Lock() 
Wake()
*/
    void SetLock(ITask* task)
    {
        LOG_ERROR("Task[%p]-%s[%d] is SetLock in waitersqueue", task, task->GetName().c_str(), task->GetWorkflowId());
        assert(mWaiters.push(WaiterType(task, true)));
    }


//后继节点调用(有可能比前驱节点先运行, 或者前驱节点运行结束之后才运行)
    bool LockShared() {
            //当前驱且节点运行结束后&&没有NextFlow Task时， 后继节点LockShare会立即返回
        if (!mWriter.load(std::memory_order_acquire) && mWaiters.empty()) {
            //++mReaders; //wake后继节点没有被resume，但是mReaders已经自增过,不为零
            LOG_ERROR("No writers && No waiters~~~~for Task-%s  ==> Acquire Shared", GetCurrentTask()->GetName().c_str());
            return true;
        }

//        LOG_WARN("ReadTask[%p]-%s[%d] suspend", GetCurrentTask(), GetCurrentTask()->GetName().c_str(), 
//                                    GetCurrentTask()->GetWorkflowId());
//        pTask->Suspend(TaskStatus::SUSPEND_BY_READ);

        return false;
    }


//每个节点DoProcess前都调用Lock
    bool Lock(ITask* task) 
    {
        if (mReaders.load(std::memory_order_acquire)==0 && !mWriter.load()) {
            std::atomic_thread_fence(std::memory_order_acquire);
            assert(!mWaiters.empty());
            auto waiter = mWaiters.front();
            if(waiter.IsWriter && task != waiter.pTask)
            {
                LOG_WARN("Worker-%d %s[%d] > Waiters.front-%s[%d]-%p", GetWorker()->GetId(), task->GetName().c_str(), task->GetWorkflowId(),
                    mWaiters.front().pTask->GetName().c_str(), mWaiters.front().pTask->GetWorkflowId(), waiter.pTask);
/*
                auto pNextTask = waiter.pTask;
                if(pNextTask->IsSuspendWrite())
                {
                    LOG_DEBUG("Wakeup WRITETask-%s[%d] in Lock()", pNextTask->GetName().c_str(), pNextTask->GetWorkflowId());
                    mWriter.store(true, std::memory_order_release);
                    EnqueueTask(pNextTask);
                }
*/
                return false;
            }
            mWriter.store(true, std::memory_order_release);
            return true;
        }
        //pTask->Suspend(TaskStatus::SUSPEND_BY_WRITE);
        return false;
    }

//后继节点结束后,释放掉本身对应的mReaders(无论是否调用过LockShared)计数
//除非Task不存在，自然不会Unlock，后继节点也不应该UnlockShared
    void UnlockShared()
    {
        if(1 == mReaders.fetch_sub(1))
        {
          //如果存在NextFlowTask，则Enqueue
            Wake();
        }
    }

//当前节点DoProcess结束就可以调用Unlock(即使Unlock后，Task[Nextflow] 依然会被mReaders阻止)
    void Unlock(ITask* task)
    {
        assert(task == mWaiters.front().pTask);
        mWaiters.pop();
        std::atomic_thread_fence(std::memory_order_release);
        LOG_DEBUG("Worker-%d pop WRITETask-%s[%d]-%p", GetWorker()->GetId(),
                    task->GetName().c_str(), task->GetWorkflowId(), task);
        mWriter.store(false);  //观测到mWriter==false ==> 当前Task已经从mWaiters中pop出来
        Wake();        
    }

private:
    //如果后继节点没有调用前驱的GetValue, 则不用唤醒 NextTask?
    void Wake()
    {
        if(mWaiters.empty())
        {
            LOG_DEBUG("Worker-%d mWaiters.empty(), nothing to wake", GetWorker()->GetId())
            return;
        }
        while(!mWaiters.empty())
        {
            
            WaiterType waiter = mWaiters.front();
            if(waiter.IsWriter)
            {
                if(mReaders.load(std::memory_order_acquire) == 0)
                {
                    /*此时可能有多个WriteTask正在竞争Lock锁，只有与当前Task相邻的才能获取到*/
                    //唤醒下一个flowID对应的Task
                    auto pNextTask = waiter.pTask;
                  
                    if(pNextTask->IsSuspendWrite())
                    {
                        LOG_DEBUG("Wakeup WRITETask-%s[%d]", pNextTask->GetName().c_str(), pNextTask->GetWorkflowId());
                        mWriter.store(true, std::memory_order_release);
                        EnqueueTask(pNextTask);
                    }

                 }

                break; //Writer唤醒后就不能有Reader了
            }
            else
            {
                mReaders.fetch_add(1); //无论ReadSuspend发生过没有，To prevent Task in NextFlow
                mWaiters.pop();

                if(mReaders.load() < 1)
                    LOG_ERROR("When %s is poped mReaders=%d", waiter.pTask->GetName().c_str(), mReaders.load());
                
                ITask* TaskSuspendByRead = waiter.pTask;

                             
                //避免临界区!!!! if(IsSuspendRead())判断时，Suspend还没有调用
                //GetValue在其他线程没有被调用，但是在当前continue后被调用, 因mWriter存在而休眠
                if(!TaskSuspendByRead->IsSuspendRead())
                    TaskSuspendByRead->TryToResume();
                if(!TaskSuspendByRead->IsSuspendRead()) //后继节点没有调用前驱的GetValue（1.Runtime没有需要，2.后继还没有开始运行）
                    continue;

                //会丢失因Read而suspend的task
                EnqueueTask(TaskSuspendByRead);
            }
        }
    }


/*
    void WakeNextTask(TaskCircularBuffer::iterator&& NextWriter)
    {
        assert(NextWriter->IsWriter);
        
        auto pNextTask = NextWriter->pTask;
        if(pNextTask->IsSuspendWrite())
        {
            pNextTask->SetReady();
            auto fromWorker = GetCurrentTask()->GetWorkerId();
            GetWorker(pNextTask->GetWorkerId())->Enqueue(fromWorker, pNextTask);
        }
    }
    
   //当前节点DoProcess结束，调用WakeShared
   //当前节点唤醒因Read而Suspend的后继节点
    void WakeShared() {
        mReaders++;
        for(auto TaskSuspendByRead=mWaiters.begin(); TaskSuspendByRead!=mWaiters.end();)
        {
         // 队列中第一个 Write节点
            if(TaskSuspendByRead->IsWriter)
                return;                

            //避免临界区!!!! if()判断时，GetValue在其他线程没有被调用，但是在当前continue后被调用, 因mWriter存在而休眠
            TaskSuspendByRead->pTask->TryToResume();
            if(!TaskSuspendByRead->pTask->IsSuspendRead()) //后继节点没有调用前驱的GetValue（1.Runtime没有需要，2.后继还没有开始运行）
                continue;

            TaskSuspendByRead->pTask->SetReady();
            auto fromWorker = GetCurrentTask()->GetWorkerId();
            GetWorker(TaskSuspendByRead->pTask->GetWorkerId())->Enqueue(fromWorker, TaskSuspendByRead->pTask);
        }

    }
*/

private:
    WaiterBufferType mWaiters;
    std::atomic<int> mReaders{0};
    std::atomic<bool> mWriter;
          
};


const int32_t NoRaiseSuccessor = -1;
class DEventNode
{
public:
    void Signal();

 //   SharedMutex& GetMutex( void ) { return mMutex; }

    const dvector<DEventNode*>& GetSuccessors( void ) const { return mSuccessors; }
    const dvector<DEventNode*>& GetPrecursors( void ) const { return mPrecursors; }

    void AddPrecursor(DEventNode* pNode) {mPrecursors.push_back(pNode);}
    void AddSuccessor(DEventNode* pNode) {mSuccessors.push_back(pNode);}

    void SetShared(ITask* task) 
    {
        if(task->GetWorkflowId() > mLastDispatchedflowId)
        {
            LOG_ERROR("SetShared: flowID doesn't match shared[%d] != %s[%d]", task->GetWorkflowId(), mName.c_str(), mLastDispatchedflowId);
            return ;
        }
        return mMutex.SetShared(task);
    }
    
    void SetLock(ITask* task) {return mMutex.SetLock(task);}

//Prevent in RemotePtr
    bool LockShared() 
    {
        if(GetCurrentTask()->GetWorkflowId() > mLastDispatchedflowId)
        {
            LOG_ERROR("LockShared: flowID doesn't match shared[%d] != %s[%d]", GetCurrentTask()->GetWorkflowId(), mName.c_str(), mLastDispatchedflowId);
            return true;
        }
        return mMutex.LockShared();
    }

    bool Lock(ITask* task) {return mMutex.Lock(task);}

    void UnlockShared()
    {
        if(GetCurrentTask()->GetWorkflowId() > mLastDispatchedflowId)
        {
            LOG_ERROR("UnlockShared: flowID doesn't match shared[%d] != %s[%d]", GetCurrentTask()->GetWorkflowId(), mName.c_str(), mLastDispatchedflowId);
            return ;
        }
        return mMutex.UnlockShared();
    }
    void Unlock(ITask* task) {return mMutex.Unlock(task);}


    const std::string& GetName() {return mName;}
    void SetName(const std::string& name) {mName = name;}

    bool HasScheduled(WorkflowID_t workflowId);
    int32_t Process(WorkflowID_t workflowId) noexcept ;

     void SetDispatchedID(WorkflowID_t workflowId) {mLastDispatchedflowId = workflowId;}
     void SetDispatchedTask(ITask* pTask) {mLastTask = pTask;}

     WorkflowID_t GetDispatchedID() {return mLastDispatchedflowId;}
     ITask* GetDispatchedTask() {return mLastTask;}

    WorkflowID_t GetLastWorkflowId() const { return mLastWorkflowId; }
    virtual void *GetValue(WorkflowID_t flowID) {return nullptr;};
protected:
    dvector<DEventNode*> mSuccessors;
    dvector<DEventNode*> mPrecursors;
    
private:
    virtual int32_t DoProcess(WorkflowID_t workflowId) = 0;
    WorkflowID_t mLastDispatchedflowId{0};
    WorkflowID_t mLastWorkflowId{0};
    ITask* mLastTask{nullptr};
    SharedMutex mMutex;
    std::string mName;
};

class DummyNode : public DEventNode
{
public:
    int32_t DoProcess(WorkflowID_t workflowId) { std::cout<<"DummyNode is used to init spsc_queue\n"; return 0;}
};
#endif //ARAGOPROJECT_DEVENTNODE_H
