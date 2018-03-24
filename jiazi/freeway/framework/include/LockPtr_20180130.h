//
// Created by shgli on 17-10-13.
//

#ifndef ARAGOPROJECT_LOCKPTR_H
#define ARAGOPROJECT_LOCKPTR_H
#include <type_traits>
#include <types.h>
#include <cassert>
#include <string>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <array>
#include <utility>
#include <unordered_map>
#include <ITask.h>
#include <Worker.h>
#include <Context.h>
#include <mutex>

#include <atomic>
class spinlock {
private:
  typedef enum {Locked, Unlocked} LockState;
  std::atomic<LockState> state_;

public:
  spinlock(const spinlock&) = delete;
  spinlock() : state_(Unlocked) {}

  void lock()
  {
    while (state_.exchange(Locked, std::memory_order_acquire) == Locked) {
      /* busy-wait */
    }
  }
  void unlock()
  {
    state_.store(Unlocked, std::memory_order_release);
  }
};
  

template <typename T>
class LockPtr;

template <typename T>
class LockQueue
{
public:
    using LockType = typename LockPtr<T>::LockType;
    LockQueue() { Init();}
    LockQueue(const LockQueue&) = delete;
    void Init() {mLock = LockPtr<T>::LockType::FREE;}


//    bool HasWriteLock() {return mLock & LockPtr<T>::LockType::WRITE;}
//    bool HasREADLock() {return mLock & LockPtr<T>::LockType::READ;}

    void Lock (typename LockPtr<T>::LockType lockType)
    {
        mLock = mLock|lockType;
    }

    void UnlockWrite()
    {
        mLock = mLock ^ (~LockType::WRITE);
    }

    void UnlockRead()
    {
            /*
            1.The first one is always the WriteLock,
            if (LockPtr::mLinkedTasksMap[NextWorkflowID] != nullptr)
                set it as ready status, then it will be processed by Worker(
                it's never removed from Worker unless RunNode returns).

             2.The following lock are ReadLock
                Just set it as ready status.
            */
          mLock = mLock ^ (~LockType::READ);
    }

    bool IsReadLocked()
    { return mLock&LockType::READ;}

    bool IsWriteLocked()
    { return mLock&LockType::WRITE; }
private:
//The Locks is separated 2 kinds: Write+Read
    uint32_t mLock;
};

template <typename T>
class LockPtr
{

public:
     enum LockType
     {
          FREE = 0x0000,
          READ = 1,
          WRITE = 1<<1,
     };

public:

//LockPtr 与 DEventNode 循环依赖，
//先构造LockPtr，才能将pLock加入到DEventNode的依赖关系中
    LockPtr(T* impl): mImpl(impl) {}
    LockPtr(LockPtr&& other)
    {
        mSuccessorsCount = std::exchange(other.mSuccessorsCount,-1);
        mImpl = std::exchange(other.mImpl, nullptr);
    }
    
    LockPtr(const LockPtr&) = delete;
    LockPtr& operator=(const LockPtr&) = delete;

    void UpdateSuccessorsCount()
    {
        mSuccessorsCount = mImpl->GetSuccessors().size();
    }


    void Attach(WorkflowID_t flow, ITask* pTask)
    {
        mLastWorkflowId = flow;
        mIsAttached[flow] = true;
        mLinkedTasksMap[flow] = pTask;
        mActiveSuccessors[flow] = mImpl->GetSuccessors().size();
        std::atomic_thread_fence(std::memory_order_release);
        LOG_DEBUG(mLog, mImpl->GetName()<<" links to [flow="<<flow<<" pTask="<<pTask<<"] succ="<<mActiveSuccessors[flow]);
        LOG_INFO(mLog, this<<" "<<mImpl->GetName()<<" ActiveSuccessors["<<flow<<"]= "<<&mActiveSuccessors[flow]);
    }

    bool HasScheduled(WorkflowID_t flow)
    {
        return flow > mLastWorkflowId;
    }

    T* operator-> ()
    {
    //There is no way to distinguish which LockPtr need to be checked
    //如果可以知道是判断哪个LockPtr有读锁，则可以每个LockPtr设置一个读锁
    //如果不知道，则只能按照锁的类别进行区分。
        if(ReadLocked())
        {
        //The current task is suspended, it will be resumed when Control::mImpl is finished.
          GetCurrentTask()->Suspend();
        }
        return mImpl;
    }

    T* Write()
    {
        if(WriteLocked())
        {
            LOG_INFO("Task-%s will be suspended due to WriteLock flow=%d\n", GetCurrentTask()->GetName().c_str(),
                                                                     GetCurrentTask()->GetWorkflowId());
            GetCurrentTask()->Suspend();
        }

        return mImpl;
    }

    T* get()
    {
        return mImpl;
    }



private:
        std::unordered_map<WorkflowID_t, ITask*> mLinkedTasksMap;
        std::unordered_map<WorkflowID_t, LockQueue<T>> mLockQueueMap;
        std::unordered_map<WorkflowID_t, uint32_t> mActiveSuccessors;

        spinlock mSpin;
        std::unordered_map<WorkflowID_t, bool> mIsAttached;
        WorkflowID_t mLastWorkflowId{0};

public:
    ITask* operator()(WorkflowID_t flowID)
    {
        auto search = mLinkedTasksMap.find(flowID);
        if(search != mLinkedTasksMap.end())
            return search->second;
        LOG_ERROR(mLog, "Can't find Task for:"<<mImpl->GetName()<<" flow="<<flowID);
        return nullptr;
    }

    template<typename Task>
    void Lock(Task* pTask)
    {
        auto flowID = pTask->GetWorkflowID();
        mLockQueueMap[flowID].Init();
        mLockQueueMap[flowID].Lock(LockType::WRITE);
        mLockQueueMap[flowID].Lock(LockType::READ);
    };


//所有的successors结束后，LockPtr对应的Task可以归还至TaskManager
//后继节点的Task 因为Write/Read的关系而suspend，重新加入原来的Worker，更改状态为ready
    template<typename Task>
    void Unlock(Task* pTask)
    {
        auto flowID = pTask->GetWorkflowID();

        assert(mLockQueueMap.find(flowID) != mLockQueueMap.end());

        mLockQueueMap[flowID].UnlockRead();
        auto& successors = mImpl->GetSuccessors();
        for(auto pLock : successors)
        {
            ITask* pTargetTask = (*pLock)(flowID);
            if(pTargetTask->IsSuspend())
            {
                pTargetTask->SetReady();
                LOG_INFO("ReadTask-%s enqueued to worker[%d]  flow=%d\n", pTargetTask->GetName().c_str(),
                                           pTargetTask->GetWorkerId(), pTargetTask->GetWorkflowId());
                GetWorker(pTargetTask->GetWorkerId())->Enqueue(GetWorkerId(), pTargetTask);
            }
        }
        //无后继节点
        if(mSuccessorsCount == 0)
        {
            Detach(flowID);
            return ;
        }
        
        if(mActiveSuccessors[flowID] == 0)
        {
            Detach(flowID);
        }
    }

//被后继节点调用，当所有后继节点都完成后，调用Detach
//被其他线程调用
    void OnPrecursorDestroy(LockPtr<T>* pLock, WorkflowID_t flow)
    {
        LOG_INFO(mLog, mImpl->GetName()<<"::OnPrecursorDestroy: flow="<<flow);
        //std::atomic_thread_fence(std::memory_order_acquire);
        if(mActiveSuccessors.find(flow) == mActiveSuccessors.end())
        {
            LOG_ERROR(mLog, "this="<<this<<" "<<mImpl->GetName()<<" ActiveSuccessors["<<flow<<"]= "<<&mActiveSuccessors[flow]<<" dosn't exist");
            return;
        };
        mActiveSuccessors[flow]--;
        if(mActiveSuccessors[flow] == 0)
        {
             Detach(flow);
        }

    }

//(本节点Unlock && 所有后继节点已经Release) ==>当前节点可以安全的销毁了
//Release WriteLock; 通知前继节点; 释放Task到原Worker或者TaskPool
//当前Task对应的对象重新放回TaskPool
    void Detach(WorkflowID_t flowID)
    {
        {
            std::lock_guard<spinlock> guard(mSpin);
            if((mIsAttached.find(flowID) == mIsAttached.end()) || !mIsAttached[flowID])
                return ;
            mIsAttached.erase(mIsAttached.find(flowID));
        }
        
        //当前节点+后继节点组成的集合，在当前flowID全部结束后才能释放WriteLock
        auto taskInNextFlowID = flowID+1;
        mLockQueueMap[flowID].UnlockWrite();
        if(mLockQueueMap.find(taskInNextFlowID) != mLockQueueMap.end())
        {
            //now, Task in next flowID can be processed
            auto pTargetTask = mLinkedTasksMap[taskInNextFlowID];
            pTargetTask->SetReady();
            //mount the task to the original Worker Queue, resume it. 
            GetWorker(pTargetTask->GetWorkerId())->Enqueue(GetWorkerId(), pTargetTask);
        }
/*
        //通知所有前继节点
        auto& precursors = mImpl->GetPrecursors();
        for(auto pLock : precursors)
        {
            LOG_INFO(mLog, mImpl->GetName()<<" notify--> "<<pLock->mImpl->GetName());
            pLock->OnPrecursorDestroy(this, flowID);
        }
*/
        auto pAssociatedTask = mLinkedTasksMap[flowID];
        if(pAssociatedTask->IsSuspend())  //Back to Queue
        {
            pAssociatedTask->SetReady();
            GetWorker(pAssociatedTask->GetWorkerId())->Enqueue(GetWorkerId(), pAssociatedTask);

        }
        else  //Back to Pool
        {
            auto pWorker = GetWorker(pAssociatedTask->GetWorkerId());
            pWorker->BackToPool(pAssociatedTask);
        }

        LOG_INFO(mLog, "LockPtr of "<<mImpl->GetName()<<"'s Information on flow="<<flowID<<" destroyed");
        mLinkedTasksMap.erase(mLinkedTasksMap.find(flowID));
        mLockQueueMap.erase(mLockQueueMap.find(flowID));
    }

private:
    bool ReadLocked( void )
    {
        auto flowID = GetCurrentTask()->GetWorkflowId();
        assert(mLockQueueMap.find(flowID) != mLockQueueMap.end());
        return mLockQueueMap[flowID].IsReadLocked();
    }
    bool WriteLocked(void )
    {
        auto flowID = GetCurrentTask()->GetWorkflowId();
        //There is no WRITE lock before current Flow
        auto preLockQueue = mLockQueueMap.find(flowID-1);
        if (preLockQueue != mLockQueueMap.end())
            return preLockQueue->second.IsWriteLocked();
        return false;
    }

    T* mImpl;
    int32_t mSuccessorsCount{-1};

};




template <typename T>
LockPtr<T> make_locked(T* impl)
{
    LockPtr<T> l(impl);
    return l ;
}
#endif //ARAGOPROJECT_LOCKPTR_H
