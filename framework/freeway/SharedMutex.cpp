//
// Created by shgli on 17-10-12.
//

#include "SharedMutex.h"
#include "Context.h"
#include "Task.h"
#include "DEventNode.h"
#define mLog Context::GetLog()
SharedMutex::SharedMutex(DEventNode *pOwner)
:mOwner(pOwner)
{
    mWaiters.Init(8192);
    mWaitingWriterWorkflowIds.Init(4096);
}

void SharedMutex::LockShared(Task* pTask)
{
    mWaiters.Push(WaiterType(pTask, false));
//    std::atomic_thread_fence(std::memory_order_release);
}

void SharedMutex::Lock(Task* pTask)
{
    mWaitingWriterWorkflowIds.Push(pTask->GetWorkflowId());
    mWaiters.Push(WaiterType(pTask, true));
//    std::atomic_thread_fence(std::memory_order_release);
}

void SharedMutex::WaitLock4(Task* pTask)
{
    auto readPos = mWaiters.ReadPos();
    auto readers = mReaders.load(std::memory_order_relaxed);
    WaiterType* realFirst = nullptr;
    if(readers < 0)
    {
        realFirst = &mWaiters.At(readPos-readers);
    }
    else
    {
        realFirst = &mWaiters.At(readPos);
    }

    if(realFirst->pTask != pTask)
    {
        LOG_INFO(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                               << ") wait lock for node:" << mOwner->GetName() << " in Worker-"<< Context::GetWorkerId()
                               << " current first task:" << realFirst->pTask << "(node:" << realFirst->pTask->GetName()
                               << ",workflow:" << realFirst->pTask->GetWorkflowId()
                               << ") readers:" << readers);
        pTask->SetWaited(mOwner);
        pTask->Suspend4Lock();
    }
    LOG_INFO(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                           << ") get lock for node:" << mOwner->GetName() << " in Worker-"<< Context::GetWorkerId());
}

bool SharedMutex::TryLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
    auto readPos = mWaiters.ReadPos();
    auto readers = mReaders.load(std::memory_order_relaxed);
    WaiterType* realFirst = nullptr;
    if(readers < 0)
    {
        realFirst = &mWaiters.At(readPos-readers);
    }
    else
    {
        realFirst = &mWaiters.At(readPos);
    }

    bool goted = realFirst->pTask == pTask;
    //bool goted = realFirst->pTask->GetWorkflowId() >= pTask->GetWorkflowId();

    LOG_INFO(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                           << ") try lock for node:" << mOwner->GetName() << (goted ? " success" : " failed")
                           << " in Worker-"<< Context::GetWorkerId() << " current first task:"
                           << realFirst->pTask << "(node:" << realFirst->pTask->GetName() << ",workflow:" << realFirst->pTask->GetWorkflowId()
                           << ") readers:" << readers);
    return goted;
}

WorkflowID_t SharedMutex::GetFirstWaittingWriter( void )
{
    auto firstWriterWkflow = mWaitingWriterWorkflowIds.First();
    return mWaitingWriterWorkflowIds.Empty() ? -1 : firstWriterWkflow;
}

void SharedMutex::WaitSharedLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
//    mTotalLocks.load(std::memory_order_acquire);
    auto firstWriterWkflow = mWaitingWriterWorkflowIds.First();
    auto hasLock = mWaitingWriterWorkflowIds.Empty() || firstWriterWkflow > pTask->GetWorkflowId();
    if(!hasLock)
    {
        assert(pTask->GetName() != mOwner->GetName());
        LOG_INFO(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                               << ") wait shared lock for node:" << mOwner->GetName() << " in Worker-"<< Context::GetWorkerId());
        pTask->SetWaited(mOwner);
        pTask->Suspend4Shared();
    }
    LOG_INFO(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                           << ") get shared lock for node:" << mOwner->GetName() << " in Worker-"<< Context::GetWorkerId());
}

bool SharedMutex::TrySharedLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
//    mTotalLocks.load(std::memory_order_acquire);
    auto firstWriterWkflow = mWaitingWriterWorkflowIds.First();
    auto hasLock = mWaitingWriterWorkflowIds.Empty() || firstWriterWkflow > pTask->GetWorkflowId();
    LOG_INFO(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                           << ") try shared lock for node:" << mOwner->GetName() << (hasLock ? " success" : " failed")
                           << " in Worker-"<< Context::GetWorkerId() << " current first writer workflow:"
                           <<  ((mWaitingWriterWorkflowIds.Empty()) ? -1 : firstWriterWkflow));
    return hasLock;
}

void SharedMutex::UnlockShared(Task* pTask)
{
    auto prevReadCnt = mReaders.fetch_sub(1, std::memory_order_relaxed);
    LOG_INFO(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                           << ") unlock shared lock node:"
                           << mOwner->GetName() << ", has readers:" << (prevReadCnt - 1));
}

void SharedMutex::Unlock(Task* pTask)
{
    LOG_INFO(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                           << ") unlocked node:" << mOwner->GetName()
                           << " in Worker-"<< Context::GetWorkerId());

    //while(mIsInWaking.test_and_set(std::memory_order_acquire));
//    mReaders.fetch_add(1, std::memory_order_relaxed);
    int32_t finished = 0;
    while(mWaiters.First(finished).pTask != pTask)
    {
        ++finished;
        mReaders.fetch_add(1, std::memory_order_relaxed);
    }

    mWaitingWriterWorkflowIds.Pop();
    mWaiters.Skip(finished + 1);
    //std::atomic_thread_fence(std::memory_order_release);
//    mReaders.fetch_sub(1, std::memory_order_relaxed);
    Wake(pTask);

    //mIsInWaking.clear(std::memory_order_release);
}

void SharedMutex::Wake(Task* pWaker)
{
    auto skipCount = 0;
    while(mWaiters.Valid(skipCount))
    {
        auto& firstWaiter = mWaiters.First(skipCount);

        if (firstWaiter.IsWriter) {
            if (0 == mReaders.load(std::memory_order_relaxed)) {
                mWaiters.Skip(skipCount);
                skipCount = 0;
                auto pTask = firstWaiter.pTask;
                LOG_INFO(mLog, "task:" << pWaker << "(node:" << pWaker->GetName() << ",workflow:" << pWaker->GetWorkflowId()
                                       << ") wake up writer:"
                                       << pTask  << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                                       << ") in Worker-"<< Context::GetWorkerId());
                pTask->Enqueue(Context::GetWorkerId(), pWaker);
            }

            break;
        } else {
            mReaders.fetch_add(1, std::memory_order_relaxed);
            ++skipCount;

            auto pTask = firstWaiter.pTask;
            LOG_INFO(mLog, "task:" << pWaker << "(node:" << pWaker->GetName() << ",workflow:" << pWaker->GetWorkflowId()
                                   << ") wake up reader:"
                                   << pTask  << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId()
                                   << ") in Worker-"<< Context::GetWorkerId());
            pTask->Enqueue(Context::GetWorkerId(), pWaker);
        }
    }

    mReaders.fetch_sub(skipCount, std::memory_order_relaxed);
}

