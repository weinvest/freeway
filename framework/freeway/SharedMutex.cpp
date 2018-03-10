//
// Created by shgli on 17-10-12.
//

#include "SharedMutex.h"
#include "Context.h"
#include "ITask.h"

SharedMutex::SharedMutex(DEventNode *pOwner)
:mOwner(pOwner){

}

bool SharedMutex::LockShared(ITask* pTask)
{
    mWaiters.push_back(WaiterType(pTask, false));
    if(mWaitingWriterWorkflowIds.empty())
    {
        return true;
    }

    return false;
}

bool SharedMutex::Lock(ITask* pTask)
{
    bool empty = mWaiters.empty();
    mWaiters.push_back(WaiterType(pTask, true));
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
    assert(!mWaiters.empty());
    return mWaiters.front().pTask == pTask;
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

        Wake();
    }
}

void SharedMutex::Unlock(ITask* task)
{

    {
        mWaiters.pop_front();
        mWaitingWriterWorkflowIds.pop_front();
        LOG_DEBUG("Mutex[%p] Worker-%d pop WRITETask-%s[%d]-%p", this, GetWorker()->GetId(),
                  task->GetName().c_str(), task->GetWorkflowId(), task);

        Wake();
    }
}

bool SharedMutex::Wake()
{
    if(mWaiters.empty())
    {
        LOG_DEBUG("Worker-%d mWaiters[%p].empty(), nothing to wake", GetWorker()->GetId(), this);
        return false;
    }

    while(!mWaiters.empty())
    {
        auto& firstWaiter = mWaiters.front();
        if(firstWaiter.IsWriter)
        {
            if (0 == mReaders)
            {
                Enqueue(GetWorkerId(), mOwner, firstWaiter.pTask);
                mWaiters.pop_front();

                return true;
            }
            break;
        }
        else
        {
            ++mReaders;
            Enqueue(GetWorkerId(), mOwner, firstWaiter.pTask);
            mWaiters.pop_front();
        }
    }

    return false;
}