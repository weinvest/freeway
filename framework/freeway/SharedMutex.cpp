//
// Created by shgli on 17-10-12.
//

#include "SharedMutex.h"
#include "Context.h"
#include "Task.h"

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
        pTask->SetWaited(mOwner);
        pTask->Suspend();
    }
}

bool SharedMutex::TryLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
    return mWaiters.First().pTask == pTask;
}

void SharedMutex::WaitSharedLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
    auto& first = mWaiters.First();
    auto hasLock = !first.IsWriter && pTask->GetWorkflowId() <= first.pTask->GetWorkflowID();
    if(!hasLock)
    {
        pTask->SetWaited(mOwner);
        pTask->Suspend();
    }
}

bool SharedMutex::TrySharedLock4(Task* pTask)
{
//    std::atomic_thread_fence(std::memory_order_acquire);
    auto& first = mWaiters.First();
    auto hasLock = !first.IsWriter && pTask->GetWorkflowId() <= first.pTask->GetWorkflowID();
    return hasLock;
}

void SharedMutex::UnlockShared(Task* pTask)
{
    LOG_DEBUG("Mutex[%p] Worker-%d releases READTask-%s[%d] mReaders' count(mReaders=%d) in UnlockShared",
              this, GetWorkerId(), GetCurrentTask()->GetName().c_str(), GetCurrentTask()->GetWorkflowId(), mReaders.load());

    auto prevReadCnt = mReaders.fetch_sub(1);
    if((prevReadCnt < 2) && !mIsInWaking.test_and_set())
    {
        //prevReadCnt == 1, 可能没有其他reader了
        //prevReadCnt <= 0, 说明漏Wake了
        mWaiters.Skip(mSkipCount);
        Wake();
        mIsInWaking.clear();
    }
}

void SharedMutex::Unlock(Task* task)
{
    mWaiters.Pop();
    mWaitingWriterWorkflowIds.pop_front();
//    std::atomic_thread_fence(std::memory_order_release);
    LOG_DEBUG("Mutex[%p] Worker-%d pop WRITETask-%s[%d]-%p", this, GetWorker()->GetId(),
              task->GetName().c_str(), task->GetWorkflowId(), task);

    Wake();
}

void SharedMutex::Wake()
{
    auto skipCount = 0;
    while(mWaiters.Valid(skipCount))
    {
        auto& firstWaiter = mWaiters.First(skipCount);

        if(LIKELY(!firstWaiter.IsNull())) {
            if (firstWaiter.IsWriter) {
                if (0 == mReaders) {
                    mWaiters.Skip(skipCount);
                    auto pTask = firstWaiter.pTask;
                    pTask->Enqueue(Context::GetWorkerId(), mOwner);
                    break;
                }
                break;
            } else {
                ++mReaders;
                auto pTask = firstWaiter.pTask;
                pTask->Enqueue(Context::GetWorkerId(), mOwner);
            }
        } else{
            int i = 0;
            ++i;
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
