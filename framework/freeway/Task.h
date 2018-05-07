//
// Created by shgli on 17-9-27.
//

#pragma once

#include <atomic>
#include <boost/context/all.hpp>
#include <string>
#include <unordered_map>
#include <set>
#include "framework/freeway/types.h"


namespace ctx = boost::context;
class DEventNode;
class DummyNode;

class Worker;
class Task
{
public:
    Task();

    Task(Task&& other) = delete;
    Task(const Task&) = delete;
    ~Task();

    int32_t GetWorkflowId() const { return mWorkflowId; }
    int32_t GetWorkerId() const;
    Worker* GetWorker() const { return mWorker; }
    void SetWorker(Worker* pWorkder) { mWorker = pWorkder; }
    DEventNode* GetNode() { return mNodePtr; }

    void Update(WorkflowID_t flow, DEventNode* pNode);

    int32_t GetWaitingLockCount( void ) const { return mWaitingLockCount.load(std::memory_order_relaxed); }

    int32_t GetResult( void ) const { return mResult; }
    DEventNode* GetWaited() { return mWaited; }

    void SetWaited(DEventNode* pWaited) { mWaited = pWaited; }

    bool IsWaitting( void ) const { return nullptr != mWaited; }
    bool IsWaittingLock( void ) const { return mWaited == mNodePtr; }

    void DecreaseWaitingLockCount( void ) { mWaitingLockCount.fetch_sub(1, std::memory_order_release); }
    void IncreaseWaitingLockCount( void ) { mWaitingLockCount.fetch_add(1, std::memory_order_relaxed); }

    void SetAcceptTrigger(bool accept) { mIsAcceptTrigger = accept; }
    bool IsSchedulable( void ) const { return mIsAcceptTrigger || 0 == mWaitingLockCount.load(std::memory_order_relaxed); }

    const std::string& GetName( void );

    void Resume( void );

    void Suspend4Lock(void);
    void Suspend4Shared( void );

    bool TryLock( void );
    bool TrySharedLock( void );
    void WaitSharedLock(DEventNode *pWaited);
    void SetId(int32_t id) { mId = id; }

    void Enqueue(int32_t from, Task *pWho);

    void CompleteDeffered(DEventNode* pNode);

    Task* Next( void ) { return mNext; }
    Task* Prev( void ) { return mPrev; }

    void Pending4Lock( void );
private:
    Task& operator =(const Task&) = delete;
    void RunNode( void );
    void Run( void );

    DEventNode* mNodePtr {nullptr};
    WorkflowID_t mWorkflowId{0};
    int32_t mId{-1};

    friend class TaskList;

    Worker* mWorker{nullptr};
    DEventNode* mWaited{nullptr};
    Task* mNext{nullptr};
    Task* mPrev{nullptr};
    std::set<DEventNode*> mDeferred;

    ctx::continuation mMainContext;
    ctx::continuation mTaskContext;

    int32_t mResult{-1};

    std::atomic_int mWaitingLockCount{0};
    bool mIsAcceptTrigger{false};
//#ifdef DEBUG
//    int32_t mLastResumeWkflowId{-1};
//    char mLastResumeThreadName[32];
//    int64_t mLastResumeTime{0};
//    int32_t mLastSuspendWkflowId{-1};
//    bool mLastSuspendWaitLock{false};
//    int64_t mLastSuspendTime{0};
//    char mLastSuspendThreadName[32];
//#endif
};

struct TaskCompare
{
    bool operator() (Task* pTask1, Task* pTask2)
    {
        return (pTask1->GetWorkflowId() > pTask2->GetWorkflowId())
               || (pTask1->GetWaitingLockCount() > pTask2->GetWaitingLockCount());
    }
};


