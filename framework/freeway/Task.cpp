//
// Created by shgli on 17-9-27.
//

#include <framework/freeway/Worker.h>
#include "Task.h"
#include "SharedMutex.h"
#include "DEventNode.h"
#include "Context.h"
using namespace boost::context;

#include <iostream>
Task::Task()
{
    mTaskContext =  ctx::callcc( [this](ctx::continuation && from)
                                 {
                                     mMainContext = from.resume();
//                                     std::cout << "task begin runnode\n";
                                     RunNode();
//                                     std::cout << "task after runnode\n";
                                     return std::move(mMainContext);  //此时Task必须有效
                                 });
//    std::cout << "Task construct:" << this <<"\n";
}

int32_t Task::GetWorkerId() const {return mWorker->GetId();}

void Task::Update(WorkflowID_t flow, Worker* worker, DEventNode* pNode)
{
    mWorkflowId = flow;
    mWorker = worker;
    mWaited = nullptr;
    mNodePtr = pNode;
    mWaitingLockCount = pNode->GetPrecursors().size();
    mLevel = 0;
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

void Task::Suspend(void)
{
    mMainContext = mMainContext.resume();
}

void Task::Resume( void )
{
//Entry for Worker's ReadyTask(After Enqueued by Dispatcher or Wakeup by other worker)
    std::cout << "Task resume:" << this << "\n";
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

        mNodePtr->Process(this, mWorkflowId);

        Context::SwitchOut();
    }
}
