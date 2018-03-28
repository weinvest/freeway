//
// Created by shgli on 17-9-27.
//

#pragma once

#include <array>
#include <queue>
#include <memory>
#include <condition_variable>

#include "common/DSpscQueue.hpp"
#include <framework/freeway/Task.h>
#include <framework/freeway/TaskList.h>
class Dispatcher;
class Worker
{
public:
    Worker(Dispatcher* pDispatcher, WorkerID_t id, int32_t workerCount);
    ~Worker( void );
    bool Initialize( void );

    void Enqueue(WorkerID_t fromWorker, void* pWho, Task* pTask);

    void Push2WaittingList(Task* pTask);

    void WaitStart( void );
    void Start( void );
    void Run( void );
    void Stop( void );

    bool IsInitialized( void ) const { return mInitialized; }
    WorkerID_t GetId( void ) const { return mId; }
    int32_t GetWorkerCount( void ) const { return mWorkerCount; }
    Dispatcher* GetDispatcher( void ) { return mDispatcher; }
    Task* AllocateTaskFromPool(WorkflowID_t flow, DEventNode* pNode);

    struct alignas(8) TaskPair
    {
        void* waited{nullptr};
        Task* task{nullptr};

        inline bool IsNull( void ) const { return nullptr == task; }
        inline void Reset( void ) { waited = nullptr; task = nullptr; }
        static inline TaskPair Null( void ) { return TaskPair(); }

        friend bool operator == (const TaskPair& lhs, const TaskPair& rhs);
        friend bool operator != (const TaskPair& lhs, const TaskPair& rhs);

    };
private:
    void CheckLostLamb(void);


    const WorkerID_t mId;
    const int32_t mWorkerCount;
    const int32_t mQueueCount;

    using PendingTaskQueue = DSpscQueue<TaskPair>;
    PendingTaskQueue* mPendingTasks;

    std::priority_queue<Task*, std::vector<Task*>, TaskCompare> mReadyTasks;

    static const int32_t TASK_POOL_SIZE = 8192;
    using TaskPool = std::array<Task, TASK_POOL_SIZE>;
    TaskPool *mTaskPool;
    int32_t mNextTaskPos{0};
    TaskList mWaittingTasks;

    std::mutex  mRuningMutex;
    std::condition_variable mRuningCond;
    bool mIsRuning;
    bool mInitialized{false};
    Dispatcher* mDispatcher{nullptr};

};
