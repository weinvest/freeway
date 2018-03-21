//
// Created by shgli on 17-9-27.
//

#include "Worker.h"
#include "Context.h"
#include <exception>
#include <boost/exception/all.hpp>

#include <iostream>

#include <framework/freeway/Task.h>
bool operator == (const Worker::TaskPair& lhs, const Worker::TaskPair& rhs)
{
    return lhs.waited == rhs.waited && lhs.task == rhs.task;
}

bool operator != (const Worker::TaskPair& lhs, const Worker::TaskPair& rhs)
{
    return !(lhs == rhs);
}

Worker::Worker(WorkerId id, int32_t workerCount)
        :mId(id)
        ,mWorkerCount(workerCount)
        ,mQueueCount(workerCount+1)
        ,mIsRuning(false)
{
}

Worker::~Worker( void )
{
    delete[] mPendingTasks;
    delete mTaskPool;
}

bool Worker::Initialize( void )
{
    mPendingTasks = new PendingTaskQueue[mQueueCount];
    for(int i=0; i<mQueueCount; i++) {
        mPendingTasks[i].Init(1024, TaskPair{0, nullptr});
    }

    mTaskPool = new TaskPool();

    return true;
}

Task* Worker::AllocateTaskFromPool(WorkflowID_t flow, Worker* pWorker, DEventNode* pNode)
{
    auto pTask = &(mTaskPool->at((mNextTaskPos++) % TASK_POOL_SIZE));
    std::cout << "allocate task:" << pTask << "\n";
    pTask->Update(flow, pWorker, pNode);
    return pTask;
}

void Worker::WaitStart( void ) {
    std::unique_lock<std::mutex> lock(mRuningMutex);
    mRuningCond.wait(lock, [this]() { return mIsRuning; });
}

void Worker::Start( void )
{
    mIsRuning = true;
    mRuningCond.notify_all();
}
//Called by other threads
void Worker::Enqueue(WorkerID_t fromWorker, void* pWho, Task* pTask)
{
    LOG_WARN("Task[%p]-%s is enqueued to worker-%d by %d, flow=%d", pTask, pTask->GetName().c_str(), mId, fromWorker, pTask->GetWorkflowId());
    auto& pTaskQueue = mPendingTasks[fromWorker];
    pTaskQueue.Push({pWho, pTask});
}


void Worker::Run( void )
{
    auto push2ReadyQueue = [this](const TaskPair& taskPair)
    {
        if(taskPair.task->GetWaited() == taskPair.waited)
        {
            mReadyTasks.push(taskPair.task);
        }
    };

    while (mIsRuning)
    {
        for(WorkerId fromWorker = 0; fromWorker < mQueueCount; ++fromWorker)
        {
            auto& pTaskQueue = mPendingTasks[fromWorker];
            pTaskQueue.consume_all(push2ReadyQueue);
        }

        while(!mReadyTasks.empty())
        {
            Task* pTask = mReadyTasks.top();
            mReadyTasks.pop();
        //    printf("=================Worker[%d] get %s\n", mId, pTask->GetName().c_str());
            try
            {
                Context::SetCurrentTask(pTask);
                pTask->Resume();
            }
            catch(const boost::exception& ex)
            {
                LOG_ERROR("%s %s %s", pTask->GetName().c_str(), " exception at Exec:", boost::diagnostic_information(ex).c_str());
            }
            catch(const std::exception& ex)
            {
                LOG_ERROR(" exception at Exec: %d", __LINE__);
            }
            catch(...)
            {
                LOG_ERROR("%s %s", pTask->GetName().c_str(), " exception at Exec");
            }
        }
    }
}

void Worker::Stop( void )
{
    mIsRuning = false;
}
