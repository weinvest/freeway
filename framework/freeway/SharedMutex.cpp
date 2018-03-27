//
// Created by shgli on 17-10-12.
//

#include "SharedMutex.h"
#include "Context.h"
#include "Task.h"

SharedMutex::SharedMutex(DEventNode *pOwner)
:mOwner(pOwner)
{
    mWaiters.Init(8192, WaiterType());
}

bool SharedMutex::LockShared(Task* pTask)
{
    mWaiters.Push(WaiterType(pTask, false));
    if(mWaitingWriterWorkflowIds.empty())
    {
        return true;
    }

    return false;
}

bool SharedMutex::Lock(Task* pTask)
{
    bool empty = mWaiters.Empty();
    mWaiters.Push(WaiterType(pTask, true));
    mWaitingWriterWorkflowIds.push_back(pTask->GetWorkflowId());

    return empty;
}

void SharedMutex::WaitLock4(Task* pTask)
{
    assert(!mWaiters.Empty());
    if(mWaiters.First()->value.pTask != pTask)
    {
        pTask->SetWaited(mOwner);
        pTask->Suspend();
    }
}

bool SharedMutex::TryLock4(Task* pTask)
{
    auto pFirst = mWaiters.First();
    return nullptr != pFirst && pFirst->value.pTask == pTask;
}

void SharedMutex::WaitSharedLock4(Task* pTask)
{
    auto hasLock = mWaitingWriterWorkflowIds.empty() || pTask->GetWorkflowId() < mWaitingWriterWorkflowIds.front();
    if(!hasLock)
    {
        pTask->SetWaited(mOwner);
        pTask->Suspend();
    }
}

bool SharedMutex::TrySharedLock4(Task* pTask)
{
    auto itBeg = mWaitingWriterWorkflowIds.begin();
    auto hasLock = itBeg == mWaitingWriterWorkflowIds.end() || pTask->GetWorkflowId() < (*itBeg);
    return hasLock;
}

void SharedMutex::UnlockShared(Task* pTask)
{
    LOG_DEBUG("Mutex[%p] Worker-%d releases READTask-%s[%d] mReaders' count(mReaders=%d) in UnlockShared",
              this, GetWorkerId(), GetCurrentTask()->GetName().c_str(), GetCurrentTask()->GetWorkflowId(), mReaders.load());

    auto prevReadCnt = mReaders.fetch_sub(1);
    if((prevReadCnt < 2) && mIsInWaking.test_and_set())
    {
        //prevReadCnt == 1, 可能没有其他reader了
        //prevReadCnt <= 0, 说明漏Wake了
        mWaiters.Pop2(mLastLockObject);
        Wake();
        mIsInWaking.clear();
    }
}

void SharedMutex::Unlock(Task* task)
{
    mWaiters.Pop();
    mWaitingWriterWorkflowIds.pop_front();
    LOG_DEBUG("Mutex[%p] Worker-%d pop WRITETask-%s[%d]-%p", this, GetWorker()->GetId(),
              task->GetName().c_str(), task->GetWorkflowId(), task);

    Wake();
}

bool SharedMutex::Wake()
{
    if(mWaiters.Empty())
    {
        LOG_DEBUG("Worker-%d mWaiters[%p].empty(), nothing to wake", GetWorker()->GetId(), this);
        return false;
    }

    auto pCurHolder = mWaiters.First();
    while(nullptr != pCurHolder)
    {
        auto& firstWaiter = pCurHolder->value;
        if(LIKELY(mWaiters.Null() != firstWaiter)) {
            if (firstWaiter.IsWriter) {
                if (0 == mReaders) {
                    firstWaiter.pTask->Enqueue(Context::GetWorkerId(), mOwner);
                    return true;
                }
                break;
            } else {
                ++mReaders;
                firstWaiter.pTask->Enqueue(Context::GetWorkerId(), mOwner);
            }
        }

        mLastLockObject = pCurHolder;
        pCurHolder = mWaiters.Next(pCurHolder);
    }

    return false;
}