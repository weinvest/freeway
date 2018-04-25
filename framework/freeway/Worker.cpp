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
#include "DEventNode.h"
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
        ,mLog(Logger::getInstance("Worker" + std::to_string(id)))
{
    mPendingTasks = new PendingTaskQueue[mQueueCount];
    for(int i=0; i<mQueueCount; i++) {
        mPendingTasks[i].Init(8192);
    }

    mTaskPool = new TaskPool();
}

Worker::~Worker( void )
{
#if 0
    for(WorkerId fromWorker = 0; fromWorker < mQueueCount; ++fromWorker)
    {
        auto& pTaskQueue = mPendingTasks[fromWorker];
        pTaskQueue.consume_all([this](const TaskPair& taskPair){
            auto pTask = taskPair.task;
            LOG_ERROR(mLog, " found unfinished task"<< pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId() << ")");
        });
    }
#endif
    delete[] mPendingTasks;
    delete mTaskPool;
}

bool Worker::Initialize( void )
{

    for(int iTask = 0; iTask < mTaskPool->size(); ++iTask)
    {
        auto& task = mTaskPool->at(iTask);
        task.SetId(iTask);
        task.SetWorker(this);
    }
    mInitialized = true;
    return true;
}

Task* Worker::AllocateTaskFromPool(WorkflowID_t flow, DEventNode* pNode)
{
    auto pTask = &(mTaskPool->at(mNextTaskPos % TASK_POOL_SIZE));
    ++mNextTaskPos;
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
    assert(IsInitialized());
    mIsRuning = true;
    mRuningCond.notify_all();
}
//Called by other threads
void Worker::Enqueue(WorkerID_t fromWorker, DEventNode* pWho, Task* pTask)
{
    auto& pTaskQueue = mPendingTasks[fromWorker];
    pTaskQueue.Push({pWho, pTask});
}

#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
void Worker::Push2WaittingList(DEventNode* pNode)
{
    mWaittingNodes[mWaittingNodeCount] = pNode;
    ++mWaittingNodeCount;
}
#else
void Worker::Push2WaittingList(Task* pTask)
{
    mWaittingTasks.Push(pTask);
}
#endif

void Worker::Run( void )
{
    auto push2ReadyQueue = [this](const TaskPair& taskPair)
    {
        auto pTask = taskPair.task;
        if(pTask->GetWaited() == taskPair.waited)
        {
            LOG_DEBUG(mLog, "task:" << pTask << "(node:" << pTask->GetName() << ",workflow:" << pTask->GetWorkflowId() << " been wake success");
            mReadyTasks.push(pTask);
            if(nullptr != pTask->GetWaited())
            {
                TaskList::Erase(pTask);
                pTask->SetWaited(nullptr);
            }
        }
        else
        {
            pTask->CompleteDeffered(taskPair.waited);
        }
    };

    int32_t nLoop = 0;
#ifdef RUN_UNTIL_NOMORE_TASK
#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    while (LIKELY(mIsRuning || mDispatcher->IsRunning() || 0 != mWaittingNodeCount)){
#else
    while (LIKELY(mIsRuning || mDispatcher->IsRunning() || !mWaittingTasks.Empty())){
#endif
//        LOG_DEBUG(mLog, "Worker-" << mId << (mWaittingTasks.Empty() ? " no ": " has ") << " watting tasks");
#else
    while(LIKELY(mIsRuning)){
#endif
        for(WorkerId fromWorker = 0; fromWorker < mQueueCount; ++fromWorker)
        {
            auto& pTaskQueue = mPendingTasks[fromWorker];
            pTaskQueue.consume_all(push2ReadyQueue);
        }

        ++nLoop;
        //if(UNLIKELY((nLoop % 100) == 0))
        if(mReadyTasks.empty())
        {
            CheckLostLamb();
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
                Context::SetCurrentTask(nullptr);
            }
            catch(const boost::exception& ex)
            {
                LOG_ERROR(mLog, pTask->GetName() << " exception when Exec:" << boost::diagnostic_information(ex));
            }
            catch(const std::exception& ex)
            {
                LOG_ERROR(mLog, pTask->GetName() << " exception when Exec@" << __FILE__ << ":" << __LINE__);
            }
            catch(...)
            {
                LOG_ERROR(mLog, pTask->GetName() << " exception at Exec");
            }
        }
    }

    std::cout << "worker-" << mId << " stoped@" << Clock::Instance().TimeOfDay().total_microseconds() << std::endl;
}

#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
void Worker::CheckLostLamb( void )  {

    int32_t newCount = 0;
    for(auto iNode = 0; iNode < mWaittingNodeCount; ++iNode)
    {
        auto pNode = mWaittingNodes[iNode];
        auto& waittingList = pNode->GetWaittingList(mId);
        while(!waittingList.Empty())
        {
            auto pTask = waittingList.Front();
            bool gotLock = false;
            if(pTask->IsWaittingLock())
            {
                gotLock = pTask->TryLock();
            }
            else
            {
                gotLock = pTask->TrySharedLock();
            }

            if(gotLock)
            {
                pTask->SetWaited(nullptr);
                mReadyTasks.push(pTask);
                waittingList.PopFront();
            }
            break;
        }

        if(!waittingList.Empty())
        {
            mWaittingNodes[newCount] = pNode;
            newCount++;
        }
    }

    mWaittingNodeCount = newCount;
}
#else
void Worker::CheckLostLamb( void )  {
    TaskList waittingTasks = std::move(mWaittingTasks);
    Task* pTail = nullptr;
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
                mWaittingTasks.Merge(pTail, waittingTasks);
                break;
            }
            else
            {
                mWaittingTasks.Push(pTask);
                if(nullptr == pTail)
                {
                    pTail = pTask;
                }
            }
        }
    }
}
#endif

void Worker::Stop( void )
{
    mIsRuning = false;
}
