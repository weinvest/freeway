//
// Created by shgli on 17-10-12.
//

#include "SharedMutex.h"
#include "Context.h"
#include "Task.h"

#define mLog Context::GetLog()
SharedMutex::SharedMutex(DEventNode *pOwner)
:mOwner(pOwner)
{
    mWaiters.Init(8192);
}

void SharedMutex::LockShared(Task* pTask)
{
    mWaiters.Push(WaiterType(pTask, false));
//    std::atomic_thread_fence(std::memory_order_release);
}

void SharedMutex::Lock(Task* pTask)
{
    mWaiters.Push(WaiterType(pTask, true));
    mWaitingWriterWorkflowIds.push_back(pTask->GetWorkflowId());
//    std::atomic_thread_fence(std::memory_order_release);
}

void SharedMutex::WaitLock4(Task* pTask)
{
    if(mWaiters.First().pTask != pTask)
    {
        LOG_INFO(mLog, "task:" << pTask << " wait lock for node:" << mOwner->GetName() << " in worker:"<< Context::GetWorkerId());
        pTask->SetWaited(mOwner);
        pTask->Suspend();
    }
    LOG_INFO(mLog, "task:" << pTask << " get lock for node:" << mOwner->GetName() << " in worker:"<< Context::GetWorkerId());
}

bool SharedMutex::TryLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
    bool goted = mWaiters.First().pTask == pTask;
    LOG_INFO(mLog, "task:" << pTask << " try lock for node:" << mOwner->GetName() << (goted ? "success" : "failed") << " in worker:"<< Context::GetWorkerId());
    return goted;
}

void SharedMutex::WaitSharedLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
    auto& first = mWaiters.First();
    auto hasLock = !first.IsWriter && pTask->GetWorkflowId() <= first.pTask->GetWorkflowID();
    if(!hasLock)
    {
        LOG_INFO(mLog, "task:" << pTask << " wait shared lock for node:" << mOwner->GetName() << " in worker:"<< Context::GetWorkerId());
        pTask->SetWaited(mOwner);
        pTask->Suspend();
    }
    LOG_INFO(mLog, "task:" << pTask << " get shared lock for node:" << mOwner->GetName() << " in worker:"<< Context::GetWorkerId());
}

bool SharedMutex::TrySharedLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
    auto& first = mWaiters.First();
    auto hasLock = !first.IsWriter && pTask->GetWorkflowId() <= first.pTask->GetWorkflowID();
    LOG_INFO(mLog, "task:" << pTask << " try shared lock for node:" << mOwner->GetName() << (hasLock ? "success" : "failed") << " in worker:"<< Context::GetWorkerId());
    return hasLock;
}

void SharedMutex::UnlockShared(Task* pTask)
{
    auto prevReadCnt = mReaders.fetch_sub(1);
    LOG_INFO(mLog, "task:" << pTask << " unlock shared, has readers:" << (prevReadCnt - 1));
    if((prevReadCnt < 2) && !mIsInWaking.test_and_set())
    {
        //prevReadCnt == 1, 可能没有其他reader了
        //prevReadCnt <= 0, 说明漏Wake了
        LOG_INFO(mLog, "task:" << pTask << " skip " << mSkipCount << " reades and then wake up " << " in worker:"<< Context::GetWorkerId());
        mWaiters.Skip(mSkipCount);
        Wake();
        mIsInWaking.clear();
    }
}

void SharedMutex::Unlock(Task* pTask)
{
    mWaiters.Pop();
    mWaitingWriterWorkflowIds.pop_front();
//    std::atomic_thread_fence(std::memory_order_release);
    LOG_INFO(mLog, "task:" << pTask << " unlocked " << " in worker:"<< Context::GetWorkerId());
    Wake();
}

void SharedMutex::Wake()
{
    auto skipCount = 0;
    while(mWaiters.Valid(skipCount))
    {
        auto& firstWaiter = mWaiters.First(skipCount);

        if (firstWaiter.IsWriter) {
            if (0 == mReaders) {
                mWaiters.Skip(skipCount);
                auto pTask = firstWaiter.pTask;
                LOG_INFO(mLog, " wake up writer:" << pTask  << " in worker:"<< Context::GetWorkerId());
                pTask->Enqueue(Context::GetWorkerId(), mOwner);
            }
            break;
        } else {
            ++mReaders;
            auto pTask = firstWaiter.pTask;
            LOG_INFO(mLog, " wake up reader:" << pTask  << " in worker:"<< Context::GetWorkerId());
            pTask->Enqueue(Context::GetWorkerId(), mOwner);
        }
        ++skipCount;
    }

    mSkipCount = skipCount;
}

Task* SharedMutex::FirstWaiter( void )
{
    auto& first = mWaiters.First();
    return first.pTask;
}
