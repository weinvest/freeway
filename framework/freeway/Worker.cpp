//
// Created by shgli on 17-9-27.
//

#include "Worker.h"
#include "Context.h"
#include <exception>
#include <boost/exception/all.hpp>
#include <iostream>
#include <framework/freeway/Task.h>
#include <clock/Clock.h>
#include "Dispatcher.h"
bool operator == (const Worker::TaskPair& lhs, const Worker::TaskPair& rhs)
{
    return lhs.waited == rhs.waited && lhs.task == rhs.task;
}

bool operator != (const Worker::TaskPair& lhs, const Worker::TaskPair& rhs)
{
    return !(lhs == rhs);
}

Worker::Worker(Dispatcher* pDispatcher, WorkerId id, int32_t workerCount)
        :mId(id)
        ,mWorkerCount(workerCount)
        ,mQueueCount(workerCount+1)
        ,mIsRuning(false)
        ,mDispatcher(pDispatcher)
{
    mPendingTasks = new PendingTaskQueue[mQueueCount];
    for(int i=0; i<mQueueCount; i++) {
        mPendingTasks[i].Init(1024, TaskPair{0, nullptr});
    }

    mTaskPool = new TaskPool();
}

Worker::~Worker( void )
{
    delete[] mPendingTasks;
    delete mTaskPool;
}

bool Worker::Initialize( void )
{

    for(int iTask = 0; iTask < mTaskPool->size(); ++iTask)
    {
        auto& task = mTaskPool->at(iTask);
        task.SetWorker(this);
        task.Resume();
    }
    mInitialized = true;
    return true;
}

Task* Worker::AllocateTaskFromPool(WorkflowID_t flow, Worker* pWorker, DEventNode* pNode)
{
    auto pTask = &(mTaskPool->at((mNextTaskPos++) % TASK_POOL_SIZE));
//    std::cout << "allocate task:" << pTask << "\n";
    pTask->Update(flow, pNode);
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

void Worker::Push2WaittingList(Task* pTask)
{
    mWaittingTasks.Push(pTask);
}

void Worker::Run( void )
{
    auto push2ReadyQueue = [this](const TaskPair& taskPair)
    {
        if(taskPair.task->GetWaited() == taskPair.waited)
        {
            taskPair.task->SetWaited(nullptr);
            mReadyTasks.push(taskPair.task);
        }
    };
#ifdef RUN_UNTIL_NOMORE_TASK
    bool bye = true;
    while (mIsRuning || mDispatcher->IsRunning() || !bye){
        bye = true;
#else
    while(LIKELY(mIsRuning)){
#endif
        for(WorkerId fromWorker = 0; fromWorker < mQueueCount; ++fromWorker)
        {
            auto& pTaskQueue = mPendingTasks[fromWorker];
            pTaskQueue.consume_all(push2ReadyQueue);
        }

        CheckLostLamb();

        while(!mReadyTasks.empty())
        {
#ifdef RUN_UNTIL_NOMORE_TASK
            bye = false;
#endif
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

    std::cout << "worker-" << mId << "stoped@" << Clock::Instance().TimeOfDay().total_microseconds() << "\n";
}

void Worker::CheckLostLamb( void )  {
    TaskList waittingTasks = std::move(mWaittingTasks);
    while(!waittingTasks.Empty()) {
        auto pTask = waittingTasks.Pop();
        if(pTask->IsWaitting())
        {
            bool gotLock = false;
            if(pTask->IsWaittingLock())
            {
                gotLock = pTask->TryLock();
            } else
            {
                gotLock = pTask->TrySharedLock();
            }

            if(gotLock)
            {
                pTask->SetWaited(nullptr);
                mReadyTasks.push(pTask);
            }
            else
            {
                mWaittingTasks.Push(pTask);
            }
        }
    }
}

void Worker::Stop( void )
{
    mIsRuning = false;
}
