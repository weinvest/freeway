//
// Created by shgli on 17-10-12.
//

#include "SharedMutex.h"
#include "Context.h"
#include "Task.h"

SharedMutex::SharedMutex(DEventNode *pOwner)
:mOwner(pOwner)
{
    mWaiters.Init(128, WaiterType());
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
        Context::SwitchOut();
    }
}

bool SharedMutex::TryLock4(Task* pTask)
{
    auto pFirst = mWaiters.First();
    return nullptr == pFirst || pFirst->value.pTask == pTask;
}

void SharedMutex::WaitSharedLock4(Task* pTask)
{
    auto hasLock = mWaitingWriterWorkflowIds.empty() || pTask->GetWorkflowId() < mWaitingWriterWorkflowIds.front();
    if(!hasLock)
    {
        pTask->SetWaited(mOwner);
        Context::SwitchOut();
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
    if(((1 ==  prevReadCnt)||(0 == prevReadCnt)) && mIsInWaking.test_and_set())
    {
        //如果存在NextFlowTask，则Enqueue
        mWaiters.Pop2(mLastLockObject);
        Wake();
        mIsInWaking.clear();
    }
}

void SharedMutex::Unlock(Task* task)
{

    {
        mWaiters.Pop();
        mWaitingWriterWorkflowIds.pop_front();
        LOG_DEBUG("Mutex[%p] Worker-%d pop WRITETask-%s[%d]-%p", this, GetWorker()->GetId(),
                  task->GetName().c_str(), task->GetWorkflowId(), task);

        Wake();
    }
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
                    Context::Enqueue(Context::GetWorkerId(), mOwner, firstWaiter.pTask);

                    return true;
                }
                break;
            } else {
                ++mReaders;
                Context::Enqueue(Context::GetWorkerId(), mOwner, firstWaiter.pTask);
            }
        }

        mLastLockObject = pCurHolder;
        pCurHolder = mWaiters.Next(pCurHolder);
    }

    return false;
}