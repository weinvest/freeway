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

class Worker
{
public:
    Worker(WorkerID_t id, int32_t workerCount);
    ~Worker( void );
    bool Initialize( void );

    void Enqueue(WorkerID_t fromWorker, void* pWho, Task* pTask);

    void WaitStart( void );
    void Start( void );
    void Run( void );
    void Stop( void );

    WorkerID_t GetId( void ) const { return mId; }
    int32_t GetWorkerCount( void ) const { return mWorkerCount; }

    Task* AllocateTaskFromPool(WorkflowID_t flow, Worker* pWorker, DEventNode* pNode);

    struct TaskPair
    {
        void* waited{nullptr};
        Task* task{nullptr};

        friend bool operator == (const TaskPair& lhs, const TaskPair& rhs);
        friend bool operator != (const TaskPair& lhs, const TaskPair& rhs);
    };
private:
    const WorkerID_t mId;
    const int32_t mWorkerCount;
    const int32_t mQueueCount;



    using PendingTaskQueue = DSpscQueue<TaskPair>;
    PendingTaskQueue* mPendingTasks;

    std::priority_queue<Task*, std::vector<Task*>, TaskCompare> mReadyTasks;

    static const int32_t TASK_POOL_SIZE = 4096;
    using TaskPool = std::array<Task, TASK_POOL_SIZE>;
    TaskPool *mTaskPool;
    int32_t mNextTaskPos{0};

    std::mutex  mRuningMutex;
    std::condition_variable mRuningCond;
    bool mIsRuning;
};
