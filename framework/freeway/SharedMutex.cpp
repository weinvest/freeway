//
// Created by shgli on 17-10-12.
//

#include "SharedMutex.h"
#include "Context.h"
#include "ITask.h"

SharedMutex::SharedMutex(DEventNode *pOwner)
:mOwner(pOwner)
{
    mWaiters.Init(128, WaiterType());
}

bool SharedMutex::LockShared(ITask* pTask)
{
    mWaiters.Push(WaiterType(pTask, false));
    if(mWaitingWriterWorkflowIds.empty())
    {
        return true;
    }

    return false;
}

bool SharedMutex::Lock(ITask* pTask)
{
    bool empty = mWaiters.Empty();
    mWaiters.Push(WaiterType(pTask, true));
    mWaitingWriterWorkflowIds.push_back(pTask->GetWorkflowId());

    return empty;
}

void SharedMutex::WaitLock4(ITask* pTask)
{
    assert(!mWaiters.Empty());
    if(mWaiters.First()->value.pTask != pTask)
    {
        Context::SwitchOut();
    }
}

void SharedMutex::WaitSharedLock4(ITask* pTask)
{
    auto hasLock = mWaitingWriterWorkflowIds.empty() || pTask->GetWorkflowId() < mWaitingWriterWorkflowIds.front();
    if(!hasLock)
    {
        pTask->SetWaited(mOwner);
        Context::SwitchOut();
        pTask->SetWaited(nullptr);
    }
}

void SharedMutex::UnlockShared(ITask* pTask)
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

void SharedMutex::Unlock(ITask* task)
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