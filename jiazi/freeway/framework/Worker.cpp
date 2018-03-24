//
// Created by shgli on 17-9-27.
//

#include "Worker.h"
#include "Context.h"
#include <exception>
#include <boost/exception/all.hpp>

#include <iostream>
const int32_t capacity=1024*256;

#include <Task.h>

Worker::Worker(WorkerId id, int32_t workerCount)
        :mId(id)
        ,mWorkerCount(workerCount)
        ,mQueueCount(workerCount+1)
        ,mPendingTasks(new PendingTaskQueue[mQueueCount]) //Worker + Dispatcher
        ,mTaskPool(std::make_unique<TaskManager>(512*1024))
        ,mIsRuning(true)
{
    for(int i=0; i<mQueueCount; i++)
        mPendingTasks[i].Init(capacity, new IdleTask());
}

Worker::~Worker( void )
{
    delete[] mPendingTasks;
}

//Called by other threads
void Worker::Enqueue(WorkerId fromWorker, ITask* pTask)
{
    LOG_WARN("Task[%p]-%s is enqueued to woker-%d by %d, flow=%d", pTask, pTask->GetName().c_str(), mId, fromWorker, pTask->GetWorkflowId());
    auto& pTaskQueue = mPendingTasks[fromWorker];
    pTaskQueue.Push(pTask);
}

void Worker::Enqueue(ITask* pTask)
{
    Enqueue(mId, pTask);
}

void Worker::Run( void )
{
    auto push2ReadyQueue = [this](ITask* pTask)
    {
       
        if(pTask->IsDispatched())
        {
            LOG_INFO("!!!!!!!!Worker-%d prepareContext for %s[%d]\n", 
                mId, pTask->GetName().c_str(), pTask->GetWorkflowId());
            pTask->PrepareContext();
        }
        
        if(pTask->IsReady())
        {
             
            LOG_INFO("!!!!!!!!Worker-%d push %s[%d] to ReadyTask\n", 
                mId, pTask->GetName().c_str(), pTask->GetWorkflowId());
        
            mReadyTasks.push(pTask);
            return true;
        }
        return false;
    };

    while (mIsRuning)
    {
        for(WorkerId fromWorker = 0; fromWorker < mQueueCount; ++fromWorker)
        {
            auto& pTaskQueue = mPendingTasks[fromWorker];
            pTaskQueue.consume_all(push2ReadyQueue);
        }

        while(!mReadyTasks.empty() || !mSpecualtingTasks.empty())
        {
            ITask* pTask = mReadyTasks.top();
            mReadyTasks.pop();
        //    printf("=================Worker[%d] get %s\n", mId, pTask->GetName().c_str());
            try
            {
                SetCurrentTask(pTask);
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
