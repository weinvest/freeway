//
// Created by shgli on 17-10-12.
//

#include "SharedMutex.h"
#include "Context.h"
#include "ITask.h"

SharedMutex::SharedMutex(DEventNode *pOwner)
:mOwner(pOwner)
{

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
    if(empty)
    {
        mWriter = pTask;
        return true;
    }

    return false;
}

bool SharedMutex::HasLock4(ITask* pTask)
{
    assert(!mWaiters.Empty());
    return mWaiters.First()->value.pTask == pTask;
}

bool SharedMutex::HasSharedLock4(ITask* pTask)
{
    auto hasLock = mWaitingWriterWorkflowIds.empty() || pTask->GetWorkflowId() < mWaitingWriterWorkflowIds.front();
    ++mReaders;
    return hasLock;
}

void SharedMutex::UnlockShared(ITask* pTask)
{
    LOG_DEBUG("Mutex[%p] Worker-%d releases READTask-%s[%d] mReaders' count(mReaders=%d) in UnlockShared",
              this, GetWorkerId(), GetCurrentTask()->GetName().c_str(), GetCurrentTask()->GetWorkflowId(), mReaders.load());

    if(2 == mReaders.fetch_sub(2))
    {
        //如果存在NextFlowTask，则Enqueue
        mWaiters.Pop2(mLastLockObject);
        Wake();
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
    if(mWaiters.Empty(mLastLockObject))
    {
        LOG_DEBUG("Worker-%d mWaiters[%p].empty(), nothing to wake", GetWorker()->GetId(), this);
        return false;
    }

    mLastLockObject = mWaiters.First();
    while(!mWaiters.Empty(mLastLockObject))
    {
        mLastLockObject = mWaiters.Next(mLastLockObject);
        auto& firstWaiter = mLastLockObject->value;
        if(firstWaiter.IsWriter)
        {
            if (0 == mReaders)
            {
                Enqueue(GetWorkerId(), mOwner, firstWaiter.pTask);

                return true;
            }
            break;
        }
        else
        {
            ++mReaders;
            Enqueue(GetWorkerId(), mOwner, firstWaiter.pTask);
        }
    }

    return false;
}