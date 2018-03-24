//
// Created by shgli on 17-10-13.
//

#ifndef ARAGOPROJECT_LOCKPTR_H
#define ARAGOPROJECT_LOCKPTR_H
#include <type_traits>
#include <Context.h>
#include <types.h>
#include <cassert>
#include <string>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <array>
#include <utility> 
#include <unordered_map>

class Task;
class ITask;
template <typename T>
class LockPtr;

template <typename T, size_t N>
class LockQueue
{
public:
    LockQueue() { Init();}
    LockQueue(const LockQueue&) = delete;
    void Init() {mIndex = 0;}
    
    /*
    LockQueue(std::initializer_list<Task*> list)
    {   
        assert(list.size() <= N);
        std::copy(list.begin(), list.end(), Queue.begin());
    
    } 
    */
    bool IsLocked() {return mIndex != 0;}
    void Lock (LockPtr<T>* pLock, typename LockPtr<T>::LockType lockType)
    {
        assert(mIndex < N);
        Queue.at(mIndex++)=std::make_pair(pLock, lockType);
    }
    void UnlockWrite()
    {
        
    }
    void Unlock()
    {
    }

    void Unlock(ITask* pTask)
    {
            /*
            1.The first one is always the WriteLock,
            if (LockPtr::mLinkedTasksMap[NextWorkflowID] != nullptr)
                set it as ready status, then it will be processed by Worker(
                it's never removed from Worker unless RunNode returns).

             2.The following lock are ReadLock
                Just set it as ready status.
            */
            


        assert(pTask != nullptr);
        if(pTask->IsReady())
            std::cout<<"Task-"<<pTask<<" is already READY\n";
        pTask->SetReady(true);

    }  

    void UnlockShared()
    {
        for(auto i=1; i<mIndex; i++)
        {
            Queue[i].first = nullptr;
        }
        mIndex = 0;
    }
private:
//All the count of locks= WriteLock + the number of Successors
    std::array<std::pair<LockPtr<T>*, typename LockPtr<T>::LockType>, N> Queue;
    std::size_t mIndex;
};

template <typename T>
class LockPtr
{

public:
     enum LockType
     {
          FREE = 0x0000,
          READ_LOCK = 0x0001,
          WRITE_LOCK = 0x0003,
          UPGRADE_LOCK = 0x0007
     };


public:

    LockPtr(T* impl): mImpl(impl),mSharedLockCount(0) {}
    LockPtr(LockPtr&& other) {mImpl=std::exchange(other.mImpl, nullptr);}
    LockPtr(const LockPtr&) = delete;
    LockPtr& operator=(const LockPtr&) = delete;


    void Init() {mSharedLockCount = 0;}
/*
    template <typename U>
    LockPtr(const LockPtr<U>& o) = delete;

    template <typename U>
    LockPtr(const LockPtr<U>& o)
    :mImpl(static_cast<T*>(o.mImpl))
    {
        static_assert(std::is_convertible<U, T>::value, "U should be convert to T");
    }
*/
    T* operator-> ()
    {
    //There is no way to distinguish which LockPtr need to be checked
    //如果可以知道是判断哪个LockPtr有读锁，则可以每个LockPtr设置一个读锁
    //如果不知道，则只能按照锁的类别进行区分。
        if(!HasSharedLock())
        {
        //The current task is suspended, it will be resumed when Control::mImpl
        //is finished.
          // SwitchOut(); 
        }
        return mImpl;
    }

    T* Write()
    {
        if(!HasWriteLock())
        {
//            SwitchOut(mControl);  --jiazi
        }

        return mImpl;
    }

    T* get()
    {
        return mImpl;
    }


    
    private:
        std::unordered_map<WorkflowID_t, ITask*> mLinkedTasksMap;
        std::unordered_map<WorkflowID_t, LockQueue<T, 64>> mLockQueueMap;

public:
    ITask* operator()(WorkflowID_t flowID)
    {
        auto search = mLinkedTasksMap.find(flowID);
        if(search != mLinkedTasksMap.end())
            return search->second;
        
        return nullptr;
    }
    template<typename Task>
    void Lock(Task* pTask) 
    {/*WriteLock      ---jiazi    */ 
        auto flowID = pTask->GetWorkflowID();
        assert(mLockQueueMap.find(flowID) != mLockQueueMap.end());
        mLockQueueMap[flowID].Init();
        mLockQueueMap[flowID].Lock(this, LockType::WRITE_LOCK);
    };
    
    template<typename Task>
    void SharedLock(Task* pTask)
    {
    //Set Read Flag for all the Successors
        auto flowID = pTask->GetWorkflowID();
        assert(mLockQueueMap.find(flowID) != mLockQueueMap.end());
        auto& successors = mImpl->GetSuccessors();
        for(auto pLock:successors)
        {

            mLockQueueMap[flowID].Lock(pLock, LockType::READ_LOCK);
        }
    };

    template<typename Task>
    void Unlock(Task* pTask) 
    {
        auto flowID = pTask->GetWorkflowID();
        auto taskInNextFlowID = flowID+1;
        assert(mLockQueueMap.find(flowID) != mLockQueueMap.end());
        if(mLockQueueMap.find(taskInNextFlowID) != mLockQueueMap.end())
            mLockQueueMap[flowID].Unlock(mLinkedTasksMap[taskInNextFlowID]);
        else
            mLockQueueMap[flowID].Unlock();        
    };
    
    template<typename Task>
    void UnlockShared(Task* pTask)
    {/*--jiazi*/
        auto flowID = pTask->GetWorkflowID();
        assert(mLockQueueMap.find(flowID) != mLockQueueMap.end());
        auto& successors = mImpl->GetSuccessors();
        for(auto pLock : successors)
        {
            mLockQueueMap[flowID].Unlock();
        }

    };


private:
    bool HasSharedLock( void )
    {
        auto flowID = GetCurrentTask()->GetWorkflowID();
        assert(mLockQueueMap.find(flowID) != mLockQueueMap.end());
        return mLockQueueMap[flowID].IsLocked();
    }
    bool HasWriteLock(void )
    {
        auto flowID = GetCurrentTask()->GetWorkflowID();
        return (mLockQueueMap.find(flowID-1) != mLockQueueMap.end())
    }
    
    T* mImpl;
    uint32_t mSharedLockCount;
};




template <typename T>
LockPtr<T> make_locked(T* impl)
{
    LockPtr<T> l(impl);
    return l ;
}
#endif //ARAGOPROJECT_LOCKPTR_H
