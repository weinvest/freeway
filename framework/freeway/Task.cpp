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
                                     Suspend();
//                                     std::cout << "task begin runnode\n";
                                     RunNode();
//                                     std::cout << "task after runnode\n";
                                     return std::move(mMainContext);  //此时Task必须有效
                                 });
//    std::cout << "Task construct:" << this <<"\n";
}

Task::~Task() {
    mNodePtr = nullptr;
//    std::cout << this << " dctor @" << mWorkflowId << "\n";
}
int32_t Task::GetWorkerId() const {return mWorker->GetId();}

void Task::Update(WorkflowID_t flow, DEventNode* pNode)
{
    mWorkflowId = flow;
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

#ifdef DEBUG
#include "clock/Clock.h"

void Task::Suspend4Lock( void )
{
    mLastSuspendWkflowId = mWorkflowId;
    mLastSuspendWaitLock = true;
    pthread_getname_np(pthread_self(), mLastSuspendThreadName, sizeof(mLastSuspendThreadName));
    mLastSuspendTime = Clock::Instance().TimeOfDay().total_microseconds();
    mMainContext = mMainContext.resume();
}

#endif

void Task::Resume( void )
{
//Entry for Worker's ReadyTask(After Enqueued by Dispatcher or Wakeup by other worker)
//    std::cout << "Task resume:" << this << "\n";
#if 0 //def DEBUG
    mLastResumeWkflowId = mWorkflowId;
    pthread_getname_np(pthread_self(), mLastResumeThreadName, sizeof(mLastResumeThreadName));
    mLastResumeTime = Clock::Instance().TimeOfDay().total_microseconds();
#endif
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
    int32_t runCnt = 0;
    while(true) {
        LOG_INFO("Enter coroutine Task for %s[%d] RunNode", GetName(), mWorkflowId);
//        char name[32];
//        pthread_getname_np(pthread_self(), name, sizeof(name));
//        std::cout << this << " run in thread:" << name << "@" << Clock::Instance().TimeOfDay().total_microseconds() << "\n";
        ++runCnt;
        mNodePtr->Process(this, mWorkflowId);
        mNodePtr = nullptr;
#if 0 //def DEBUG
        mLastSuspendWaitLock = false;
        mLastSuspendWkflowId = mWorkflowId;
        mLastSuspendTime = Clock::Instance().TimeOfDay().total_microseconds();
#endif
        Suspend();
    }
}

bool Task::TryLock( void )
{
    return mNodePtr->GetMutex().TryLock4(this);
}

bool Task::TrySharedLock( void )
{
    return mNodePtr->GetMutex().TrySharedLock4(this);
}