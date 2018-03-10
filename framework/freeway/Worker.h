//
// Created by shgli on 17-9-27.
//

#pragma once

#include <array>
#include <queue>
#include <memory>

#include "common/DSpscQueue.hpp"
#include <framework/freeway/Task.h>

class Worker
{
public:
    Worker(WorkerID_t id, int32_t workerCount);
    ~Worker( void );
    bool Initialize( void );

    void Run( void );
    void Enqueue(WorkerID_t fromWorker, void* pWho, ITask* pTask);

    void Stop( void );

    WorkerID_t GetId( void ) const { return mId; }
    int32_t GetWorkerCount( void ) const { return mWorkerCount; }

    ITask* AllocateTaskFromPool(WorkflowID_t flow, Worker* pWorker, DEventNode* pNode);

    struct TaskPair
    {
        void* waited{nullptr};
        ITask* task{nullptr};

        friend bool operator == (const TaskPair& lhs, const TaskPair& rhs);
        friend bool operator != (const TaskPair& lhs, const TaskPair& rhs);
    };
private:
    const WorkerID_t mId;
    const int32_t mWorkerCount;
    const int32_t mQueueCount;



    using PendingTaskQueue = DSpscQueue<TaskPair>;
    PendingTaskQueue* mPendingTasks;

    std::priority_queue<ITask*, std::vector<ITask*>, TaskCompare> mReadyTasks;

    static const int32_t TASK_POOL_SIZE = 4096;
    using TaskPool = std::array<Task, TASK_POOL_SIZE>;
    TaskPool *mTaskPool;
    int32_t mNextTaskPos{0};

    bool mIsRuning;
};
