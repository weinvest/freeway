//
// Created by shgli on 17-9-27.
//

#pragma once

#include <array>
#include <queue>
#include <memory>
#include <condition_variable>

#include "common/DSpscArray.h"
#include "framework/freeway/Task.h"
#include "framework/freeway/TaskList.h"
#include "utils/DLog.h"
class Dispatcher;
class Worker
{
public:
    Worker(Dispatcher* pDispatcher, WorkerID_t id, int32_t workerCount);
    ~Worker( void );
    bool Initialize( void );

    void Enqueue(WorkerID_t fromWorker, DEventNode* pWho, Task* pTask);
    void Dispatch(Task* pTask);
#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    void Push2WaittingList(DEventNode* pNode);
#else
    void Push2WaittingList(Task* pTask);
#endif

    void WaitStart( void );
    void Start( void );
    void Run( void );
    void Stop( void );

    bool IsInitialized( void ) const { return mInitialized; }
    WorkerID_t GetId( void ) const { return mId; }
    int32_t GetWorkerCount( void ) const { return mWorkerCount; }
    Dispatcher* GetDispatcher( void ) { return mDispatcher; }
    Task* AllocateTaskFromPool(WorkflowID_t flow, DEventNode* pNode);
    Logger& GetLog() { return mLog; }

    struct alignas(8) TaskPair
    {
        DEventNode* waited{nullptr};
        Task* task{nullptr};

        friend bool operator == (const TaskPair& lhs, const TaskPair& rhs);
        friend bool operator != (const TaskPair& lhs, const TaskPair& rhs);

    };

    void FinishATask( void ) { ++mFinishedTasks; }

    void OutputWaitingTasks( void ) { mOutputWaitingTasks = true; }
private:
    void CheckLostLamb(void);
    void DoOutputWaitingTasks( void );

    const WorkerID_t mId;
    const int32_t mWorkerCount;
    const int32_t mQueueCount;

    using DispatchedTaskQueue = DSpscArray<Task*, std::atomic_int>;
    using PendingTaskQueue = DSpscArray<TaskPair>;
    DispatchedTaskQueue mDispatchedTasks;
    PendingTaskQueue* mPendingTasks;

    std::priority_queue<Task*, std::vector<Task*>, TaskCompare> mReadyTasks;

    static const int32_t TASK_POOL_SIZE = 8192;
    using TaskPool = std::array<Task, TASK_POOL_SIZE>;
    TaskPool *mTaskPool;
    alignas(64) int32_t mNextTaskPos{0};
#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    std::array<DEventNode*, 1024> mWaittingNodes;
    int32_t mWaittingNodeCount{0};
#else
    TaskList mWaittingTasks;
#endif

#ifdef _USING_MULTI_LEVEL_UNSCHEDULABLE
    std::array<DEventNode*, 1024> mUnschedulableNodes;
    int32_t mUnschedulableNodeCount{0};
#endif

    std::mutex  mRuningMutex;
    std::condition_variable mRuningCond;
    bool mIsRuning{false};
    bool mInitialized{false};
    bool mOutputWaitingTasks{false};
    Dispatcher* mDispatcher{nullptr};
    int32_t mFinishedTasks{0};

    Logger mLog;
};
