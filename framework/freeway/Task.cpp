//
// Created by shgli on 17-9-27.
//

#include <framework/freeway/Worker.h>
#include "Task.h"
#include "SharedMutex.h"
#include "DEventNode.h"
#include "Context.h"
using namespace boost::context;

int32_t ITask::GetWorkerId() const {return mWorker->GetId();}

Task::Task()
{
    mTaskContext =  ctx::callcc( [this](ctx::continuation && from)->ctx::continuation&&
        {
            mMainContext = from.resume();
            RunNode();
            return std::move(mMainContext);  //此时Task必须有效
        });
}

/*
//Node 本身是不是需要运行 --jiazi
bool Task::IsScheduleAble( void ) const
{
    return mAccepted || 0 == mWaitingLockCount.load(std::memory_order_relaxed);
}
*/
const std::string& Task::GetName( void )
{
    return mNodePtr->GetName();
}

//只有Ready的task才可以加入Queue，因为只要加入了queue就会被立刻取出来，非init/ready状态的则被丢弃

void Task::Suspend(TaskStatus reason) {
    mMainContext = mMainContext.resume();
}

void Task::Resume( void )
{
//Entry for Worker's ReadyTask(After Enqueued by Dispatcher or Wakeup by other worker)
    if(mTaskContext) {
        mTaskContext = mTaskContext.resume();
    }
    else {
        //the mTaskContext is still resuming in other thread rather return.
        LOG_ERROR("+++++++Task[%p]-%s's context is null flow=%d", this, GetName().c_str(), mWorkflowId);
    }
}

void Task::RunNode( void )
{
    while(true) {
        LOG_INFO("Enter coroutine Task for %s[%d] RunNode", GetName().c_str(), mWorkflowId);
        if (!mNodePtr->GetMutex().HasLock4(this)) {
            SwitchOut();
        }
//run here when wakeup by Unlock/UnlockShared
        mNodePtr->Process(mWorkflowId);

        for (auto precursor : mNodePtr->GetPrecursors()) {
            LOG_INFO("Coroutine Task for %s[%d] RunNode is over, Unlockshared(%s) on Worker-%d", GetName().c_str(),
                     mWorkflowId, precursor->GetName().c_str(), mWorkerId);
            precursor->GetMutex().UnlockShared(this);
        }

        LOG_INFO("Coroutine Task for %s[%d] RunNode is over, Unlocking...", GetName().c_str(), mWorkflowId);
        mNodePtr->GetMutex().Unlock(this);

        SwitchOut();
    }
}
