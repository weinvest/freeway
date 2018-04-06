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
    mTaskContext =  ctx::callcc( [this](ctx::continuation && from)
                                 {
                                     mMainContext = from.resume();
//                                     std::cout << "task begin runnode\n";
                                     if(LIKELY(nullptr != mNodePtr)) {
                                         RunNode();
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
    mWorkflowId = flow;
    mNodePtr = pNode;
    mWaitingLockCount = pNode->GetPrecursors().size();
    mLevel = 0;
    assert(mDeferred.empty());  //说明Worker的Task Pool太小啦
}

const std::string& Task::GetName( void )
{
    return mNodePtr->GetName();
}

void Task::Suspend(void)
{
    mWorker->Push2WaittingList(this);
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
    Suspend();
}

#endif

void Task::Resume( void )
{
//Entry for Worker's ReadyTask(After Enqueued by Dispatcher or Wakeup by other worker)
//    std::cout << "Task resume:" << this << "\n";
#ifdef DEBUG
    mLastResumeWkflowId = mWorkflowId;
    pthread_getname_np(pthread_self(), mLastResumeThreadName, sizeof(mLastResumeThreadName));
    mLastResumeTime = Clock::Instance().TimeOfDay().total_microseconds();
#endif
    if(mTaskContext) {
        mTaskContext = mTaskContext.resume();
    }
    else {
        //the mTaskContext is still resuming in other thread rather return.
        LOG_ERROR(mLog, "Resume task:" << this << " when mTaskContext unresumeable");
    }
}

void Task::RunNode( void )
{
    int32_t runCnt = 0;
    while(true) {
//        char name[32];
//        pthread_getname_np(pthread_self(), name, sizeof(name));
//        std::cout << this << " run in thread:" << name << "@" << Clock::Instance().TimeOfDay().total_microseconds() << "\n";
        ++runCnt;
        mNodePtr->GetMutex().WaitLock4(this);

        mNodePtr->Process(this, mWorkflowId);

        for (auto precursor : mNodePtr->GetPrecursors()) {
            auto& mutex = precursor->GetMutex();
            if(mutex.TrySharedLock4(this))
            {
                mutex.UnlockShared(this);
            } else{
                mDeferred.insert(precursor);
            }
        }

        mNodePtr->GetMutex().Unlock(this);
        mNodePtr = nullptr;
#ifdef DEBUG
        mLastSuspendWaitLock = false;
        mLastSuspendWkflowId = mWorkflowId;
        mLastSuspendTime = Clock::Instance().TimeOfDay().total_microseconds();
#endif
        mMainContext = mMainContext.resume();
    }
}

void Task::CompleteDeffered(DEventNode* pNode)
{
    if(nullptr == mNodePtr)
    {
        if(1 == mDeferred.erase(pNode))
        {
            pNode->GetMutex().UnlockShared(this);
        }
    }
}

bool Task::TryLock( void )
{
    return mNodePtr->GetMutex().TryLock4(this);
}

bool Task::TrySharedLock( void )
{
    return ((DEventNode*)mWaited)->GetMutex().TrySharedLock4(this);
}

void Task::WaitSharedLock(DEventNode *pWaited)
{
    return pWaited->GetMutex().WaitSharedLock4(this);
}

void Task::Enqueue(int32_t from, void *pWhy)
{
    mWorker->Enqueue(from, pWhy, this);
}

