//
// Created by shgli on 17-9-27.
//

#pragma once

#include <atomic>
#include <vector>
#include <boost/context/all.hpp>
#include "ITask.h"
#include <unordered_map>


namespace ctx = boost::context;
class DEventNode;
class DummyNode;

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


class Task: public ITask
{
public:
    Task(WorkflowID_t workflowId, WorkerID_t workerId);
    Task():ITask(0,0) {}
    Task(Task&& other)
    {
        std::swap(mMainContext, other.mMainContext);
        std::swap(mTaskContext, other.mTaskContext);
        std::swap(mWorkflowId, other.mWorkflowId);
        std::swap(mWorkerId, other.mWorkerId);
        std::swap(mNodePtr, other.mNodePtr);
        std::swap(mStatus, other.mStatus);
    }
    Task(const Task&) = delete;
    Task& operator =(const Task&) = delete;

    const std::string& GetName( void ) const override;

    virtual void SetLock() override;
    virtual void SetShared(ITask* pTask) override;

    //void Acquire( void ) override;
    //void Release( void ) override;

    void Resume( void ) override;
    void PrepareContext() override;

//    bool IsScheduleAble( void ) const;

    WorkflowID_t GetWorkflowID( void ) const { return mWorkflowId; }
    void Suspend(TaskStatus reason ) override;
private:
    void RunNode( void );

    ctx::continuation mMainContext;
    ctx::continuation mTaskContext;
    bool mLockAcquired;
 //   std::atomic_int mWaitingLockCount;
 //   bool mAccepted;
};

#define ATOMIC_INDEX ;
#include <iostream>
class TaskManager
{
public:
    using Index_t=std::vector<uint32_t>;
    using TaskPool_t=std::vector<Task>;
    TaskManager(std::size_t size):
        mTotal(size)
    {
#ifndef ATOMIC_INDEX
        mFreeIndex.reserve(size);
        for(auto i=0; i<size; i++)
        {
            mFreeIndex.push_back(i);
        }

#endif
        mTaskPool.reserve(size);    
        for(auto i=0; i<size; i++)
        {
           mTaskPool.emplace_back();
        }
    }
/*Dispatcher Allocate; Worker Deallocate*/
    ITask* AllocateTask()
    {
    #ifdef ATOMIC_INDEX
        auto index = mCurrent.fetch_add(1)%mTotal;
        return &mTaskPool[index];
    #else
        ITask* pTask;
        if(mFreeIndex.empty())
        {
            std::cout<<"All the Task Objects are consumed\n";
            assert(false);
            return nullptr;
        }

        auto index = mFreeIndex.back();
        pTask = &mTaskPool[index];
        printf("Allocate %p from pool\n", pTask);
        mFreeIndex.pop_back();
        return pTask;
     #endif
    }

    void DeallocateTask(ITask* pITask)
    {
#ifdef ATOMIC_INDEX
      //  mCurrent.fetch_sub(1);
#endif
    /*
        Task* pTask = static_cast<Task*>(pITask);
        auto index = pTask - mTaskPool.data();
        assert(index < mTotal);
        if( std::find(mFreeIndex.begin(), mFreeIndex.end(), index) != mFreeIndex.end())
        {
            auto dis = std::distance(mFreeIndex.begin(), pos);
            LOG_ERROR("%p index=%d, distance=%d, mTotal=%d\n", pTask, index, dis, mTotal);
            *(int*)0x0 = 0x1234;
        }
        printf("Put %s[%p-%d] back to pool\n", pITask->GetName().c_str(), pITask, pITask->GetWorkflowId());
        mFreeIndex.push_back(index);
    */
    }
private:
    const std::size_t mTotal;
#ifdef ATOMIC_INDEX
    std::atomic<unsigned> mCurrent;
#else
    Index_t mFreeIndex;
#endif
    TaskPool_t mTaskPool;
};


