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

    int32_t GetWorkflowId() const { return mWorkflowId; }
    int32_t GetWorkerId() const;
    Worker* GetWorker() const { return mWorker; }
    DEventNode* GetNode() { return mNodePtr; }

    int32_t GetLevel() const { return mLevel; }

    void SetLevel(int32_t level) { mLevel = level; }

    void Update(WorkflowID_t flow, Worker* worker, DEventNode* pNode);

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

private:
    Task& operator =(const Task&) = delete;
    void RunNode( void );

    Worker* mWorker{nullptr};
    DEventNode* mNodePtr {nullptr};

    friend class TaskList;
    void* mWaited{nullptr};
    Task* mNext{nullptr};

    WorkflowID_t mWorkflowId{0};
    int32_t mWaitingLockCount;
    int32_t mLevel{0};

    ctx::continuation mMainContext;
    ctx::continuation mTaskContext;
    bool mLockAcquired{false};
 //   std::atomic_int mWaitingLockCount;
 //   bool mAccepted;
};

struct TaskCompare
{
    bool operator() (Task* pTask1, Task* pTask2)
    {
        return pTask1->GetWorkflowId() < pTask2->GetWorkflowId() || pTask1->GetLevel() < pTask2->GetLevel()
               || pTask1->GetWaitingLockCount() < pTask2->GetWaitingLockCount();
    }
};


