//
// Created by shgli on 17-9-27.
//

#pragma once

#include <array>
#include <queue>
#include <memory>

#include "common/DSpscQueue.hpp"
#include <Task.h>
using WorkerID_t = WorkerId;
class Worker
{
public:
    Worker(WorkerID_t id, int32_t workerCount);
    ~Worker( void );

    void Run( void );
    void Enqueue(WorkerID_t fromWorker, ITask* pTask);
    void Enqueue(ITask* pTask); //Task因调用前继而suspend时，把自身重新放回PendingTaskQueue

    void Stop( void );

    WorkerID_t GetId( void ) const { return mId; }
    int32_t GetWorkerCount( void ) const { return mWorkerCount; }

    ITask* AllocateTaskFromPool(WorkflowID_t flow, WorkerID_t idxWorker, DEventNode* pNode)
    {
         auto pTask = mTaskPool->AllocateTask();
         pTask->Update(flow, idxWorker, pNode);
         pTask->SetInit();
         std::atomic_thread_fence(std::memory_order_release);
         return pTask;
    }
    
    void BackToPool(ITask* pTask)
    {
        mTaskPool->DeallocateTask(pTask);
    }

private:
    const WorkerID_t mId;
    const int32_t mWorkerCount;
    const int32_t mQueueCount;

    using PendingTaskQueue = DSpscQueue<ITask*>;
    PendingTaskQueue* mPendingTasks;
    std::unique_ptr<TaskManager> mTaskPool;
    std::vector<ITask*> mSpecualtingTasks;

    std::priority_queue<ITask*, std::vector<ITask*>, TaskCompare> mReadyTasks;
    bool mIsRuning;
};
