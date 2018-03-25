//
// Created by shgli on 17-9-27.
//

#pragma once

#include <atomic>
#include <boost/context/all.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <framework/freeway/types.h>


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

    int32_t GetLevel() const { return mLevel; }

    void SetLevel(int32_t level) { mLevel = level; }

    void Update(WorkflowID_t flow, DEventNode* pNode);

    int32_t GetWaitingLockCount( void ) const { return mWaitingLockCount; }


    void* GetWaited() { return mWaited; }

    void SetWaited(void* pWaited) { mWaited = pWaited; }

    bool IsWaitting( void ) const { return nullptr != mWaited; }
    bool IsWaittingLock( void ) const { return mWaited == mNodePtr; }

    void DecreaseWaitingLockCount( void ) { --mWaitingLockCount; }


    const std::string& GetName( void );

    void Resume( void );

    WorkflowID_t GetWorkflowID( void ) const { return mWorkflowId; }
    void Suspend(void);

    bool TryLock( void );
    bool TrySharedLock( void );
#ifdef DEBUG
    void Suspend4Lock( void );
#endif
private:
    Task& operator =(const Task&) = delete;
    void RunNode( void );

    DEventNode* mNodePtr {nullptr};
    WorkflowID_t mWorkflowId{0};
    int32_t mWaitingLockCount{0};
    int32_t mLevel{0};

    friend class TaskList;

    Worker* mWorker{nullptr};
    void* mWaited{nullptr};
    Task* mNext{nullptr};

    ctx::continuation mMainContext;
    ctx::continuation mTaskContext;

#ifdef DEBUG
    int32_t mLastResumeWkflowId{-1};
    char mLastResumeThreadName[32];
    int64_t mLastResumeTime{0};
    int32_t mLastSuspendWkflowId{-1};
    bool mLastSuspendWaitLock{false};
    int64_t mLastSuspendTime{0};
    char mLastSuspendThreadName[32];
#endif
};

struct TaskCompare
{
    bool operator() (Task* pTask1, Task* pTask2)
    {
        return pTask1->GetWorkflowId() < pTask2->GetWorkflowId() || pTask1->GetLevel() < pTask2->GetLevel()
               || pTask1->GetWaitingLockCount() < pTask2->GetWaitingLockCount();
    }
};


