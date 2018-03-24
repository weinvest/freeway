//
// Created by shgli on 17-9-27.
//

#include <Worker.h>
#include "Task.h"
//#include "SharedMutex.h"
#include "DEventNode.h"
using namespace boost::context;


void Task::PrepareContext()
{
    SetReady();
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
const std::string& Task::GetName( void ) const
{
    return mNodePtr->GetName();
}

//只有Ready的task才可以加入Queue，因为只要加入了queue就会被立刻取出来，非init/ready状态的则被丢弃
void Task::Suspend( TaskStatus reason )
{
    if(!mMainContext)
    {
        LOG_ERROR("%s MainContext is null...", GetName().c_str());
        return;
    }
    
    if(SetSuspend(reason))
    {
        const char* str;
        if(reason == TaskStatus::SUSPEND_BY_READ)
            str = "READ";
        else if(reason == TaskStatus::SUSPEND_BY_WRITE)
            str = "WRITE";
        else
            assert(0);
        
        LOG_WARN("Task[%p]-%s %sSuspend  flow = %d", this, GetName().c_str(), str, mWorkflowId);
        mMainContext = mMainContext.resume();
    }

    //run here when resume again!!
}

void Task::SetLock()
{
    mNodePtr->SetLock(this);
    if(mLockAcquired)
        LOG_INFO(" Task-%p[%d]-%s grabs the lock by SetLock", this, mWorkflowId, GetName().c_str());
}

void Task::SetShared(ITask* pTask)
{
    mNodePtr->SetShared(pTask);
}


void Task::Resume( void )
{
//Entry for Worker's ReadyTask
    if(mTaskContext)
        mTaskContext = mTaskContext.resume();
    else
        //the mTaskContext is still resuming in other thread rather return.
        LOG_ERROR("+++++++Task[%p]-%s's context is null flow=%d", this, GetName().c_str(), mWorkflowId);

    // 从携程返回main时; 或者Task::Suspend调用后会运行至此
    if(IsStop())
    {
        GetWorker(mWorkerId)->BackToPool(this);
        //mLockPtr_p->Detach(mWorkflowId);
    }
}

void Task::RunNode( void )
{
    if(!mNodePtr->Lock(this))
    {
        Suspend(SUSPEND_BY_WRITE);
    }

//run here when wakeup following Unlock/UnlockShared
    mNodePtr->Process(mWorkflowId);
    
    LOG_INFO("Coroutine Task[%d]-%s RunNode is over, Unlocking...", mWorkflowId, GetName().c_str());
    mNodePtr->Unlock(this);
    for(auto precursor : mNodePtr->GetPrecursors())
    {
        LOG_INFO("Coroutine Task[%d]-%s RunNode is over, Unlockshared(%s)", mWorkflowId, GetName().c_str(), precursor->GetName().c_str());
        precursor->UnlockShared();
    }
    SetStop();
}
