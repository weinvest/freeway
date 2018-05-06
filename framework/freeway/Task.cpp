//
// Created by shgli on 17-9-27.
//

#include "Worker.h"
#include "Task.h"
#include "SharedMutex.h"
#include "DEventNode.h"
#include "Context.h"
using namespace boost::context;
#define mLog Context::GetLog()
Task::Task()
{
    constexpr size_t stack_size = 8192*2;
    mTaskContext =  ctx::callcc(std::allocator_arg, ctx::fixedsize_stack(stack_size), [this](ctx::continuation && from)
                                 {
                                     mMainContext = from.resume();
//                                     std::cout << "task begin runnode\n";
                                     if(LIKELY(nullptr != mNodePtr)) {
                                         Run();
                                     }
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
    assert(mDeferred.empty());  //说明Worker的Task Pool太小啦
    mWorkflowId = flow;
    mNodePtr = pNode;
    mWaitingLockCount = 0;
    mNext = mPrev = this;
    mIsAcceptTrigger = false;
}

const std::string& Task::GetName( void )
{
    return mNodePtr->GetName();
}

void Task::Suspend4Lock(void)
{
#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    auto& taskList = mWaited->mWaitingTasks[GetWorkerId()];
    auto pCurrTask = taskList.Back();
    if(taskList.TraverseEnd(pCurrTask))
    {
        mWorker->Push2WaittingList(mWaited);
        taskList.PushBack(this);
    }
    else
    {
        while(!taskList.TraverseEnd(pCurrTask) && pCurrTask->GetWorkflowId() >= GetWorkflowId())
        {
            pCurrTask = pCurrTask->mPrev;
        }
        taskList.InsertAfter(pCurrTask, this);
    }
#else
    mWorker->Push2WaittingList(this);
#endif

#ifndef PRELOCK_WHEN_RUN
    mMainContext = mMainContext.resume();
#endif
}

void Task::Suspend4Shared( void )
{
#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    auto& taskList = mWaited->mWaitingTasks[GetWorkerId()];
    auto pCurrTask = taskList.Back();
    if(taskList.TraverseEnd(pCurrTask))
    {
        mWorker->Push2WaittingList(mWaited);
        taskList.PushBack(this);
    }
    else
    {
        while(!taskList.TraverseEnd(pCurrTask) && pCurrTask->GetWorkflowId() > GetWorkflowId())
        {
            pCurrTask = pCurrTask->mPrev;
        }

        taskList.InsertAfter(pCurrTask, this);
    }
#else
    mWorker->Push2WaittingList(this);
#endif
    mMainContext = mMainContext.resume();
}
//#ifdef DEBUG
//#include "clock/Clock.h"
//
//void Task::Suspend4Lock( void )
//{
//    mLastSuspendWkflowId = mWorkflowId;
//    mLastSuspendWaitLock = true;
//    pthread_getname_np(pthread_self(), mLastSuspendThreadName, sizeof(mLastSuspendThreadName));
//    mLastSuspendTime = Clock::Instance().TimeOfDay().total_microseconds();
//    Suspend();
//}
//
//#endif

void Task::Resume( void )
{
//Entry for Worker's ReadyTask(After Enqueued by Dispatcher or Wakeup by other worker)
//    std::cout << "Task resume:" << this << "\n";
//#ifdef DEBUG
//    mLastResumeWkflowId = mWorkflowId;
//    pthread_getname_np(pthread_self(), mLastResumeThreadName, sizeof(mLastResumeThreadName));
//    mLastResumeTime = Clock::Instance().TimeOfDay().total_microseconds();
//#endif
    if(LIKELY(mTaskContext)) {
        mTaskContext = mTaskContext.resume();
    }
    else {
        //the mTaskContext is still resuming in other thread rather return.
        LOG_ERROR(mLog, "Resume task:" << this << " when mTaskContext unresumeable");
    }
}

void Task::Run( void )
{
    int32_t runCnt = 0;
    while(true) {
        ++runCnt;
        RunNode();
    }
}

void Task::RunNode( void )
{
//        char name[32];
//        pthread_getname_np(pthread_self(), name, sizeof(name));
//        std::cout << this << " run in thread:" << name << "@" << Clock::Instance().TimeOfDay().total_microseconds() << "\n";
    mWaitingLockCount.load(std::memory_order_acquire);
    if(mIsAcceptTrigger)
    {
#ifndef PRELOCK_WHEN_RUN
        mNodePtr->GetMutex().WaitLock4(this);
#endif

        mResult = mNodePtr->Process(this, mWorkflowId);
    }

    for (auto precursor : mNodePtr->GetPrecursors())
    {
        auto& mutex = precursor->GetMutex();
        if(mutex.TrySharedLock4(this))
        {
            mutex.UnlockShared(this);
        }
        else
        {
            mDeferred.insert(precursor);
        }
    }

    mNodePtr->GetMutex().Unlock(this);
    while(!mDeferred.empty())
    {
        auto pPrecursor = *(mDeferred.begin());
        auto& mutex = pPrecursor->GetMutex();
        int32_t cnt = mDeferred.erase(pPrecursor);
        assert(1 == cnt);

        mutex.WaitSharedLock4(this);
        mutex.UnlockShared(this);
    }

    mWorker->FinishATask();
//#ifdef DEBUG
//    mLastSuspendWaitLock = false;
//    mLastSuspendWkflowId = mWorkflowId;
//    mLastSuspendTime = Clock::Instance().TimeOfDay().total_microseconds();
//#endif
    mMainContext = mMainContext.resume();
}

bool Task::TryLock( void )
{
    return mNodePtr->GetMutex().TryLock4(this);
}

bool Task::TrySharedLock( void )
{
    return mWaited->GetMutex().TrySharedLock4(this);
}

void Task::WaitSharedLock(DEventNode *pWaited)
{
    return pWaited->GetMutex().WaitSharedLock4(this);
}

void Task::Enqueue(int32_t from, Task* pWho)
{
    auto pPrecessor = pWho->GetNode();
    if(pPrecessor->GetLastWorkflowId() == mWorkflowId)
    {
        if (!mIsAcceptTrigger && mNodePtr->Raise(pPrecessor, pWho->GetResult()))
        {
            mIsAcceptTrigger = true;
        }
        DecreaseWaitingLockCount();
    }

    mWorker->Enqueue(from, pPrecessor, this);
}

void Task::CompleteDeffered(DEventNode* pNode)
{
    if(1 == mDeferred.erase(pNode))
    {
        pNode->GetMutex().UnlockShared(this);
    }
}
