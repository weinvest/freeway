//
// Created by shgli on 17-9-27.
//
#include <iostream>
#include <fstream>
#include "Worker.h"
#include "Context.h"
#include <exception>
#include <boost/exception/all.hpp>
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
        ,mQueueCount(workerCount)
        ,mIsRuning(false)
        ,mDispatcher(pDispatcher)
        ,mLog(Logger::getInstance("Worker" + std::to_string(id)))
{
    mDispatchedTasks.Init(8192);

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
    auto& pTaskQueue = mPendingTasks[fromWorker-1U];
    pTaskQueue.Push({pWho, pTask});
}

void Worker::Dispatch(Task* pTask)
{
    mDispatchedTasks.Push(pTask);
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
    mWaittingTasks.PushBack(pTask);
}
#endif

void Worker::Run( void )
{
    auto push2ReadyQueue = [this](const TaskPair& taskPair)
    {
        auto pTask = taskPair.task;
        if(pTask->GetWaited() == taskPair.waited)
        {
            if(pTask->IsSchedulable())
            {
                mReadyTasks.push(pTask);
                TaskList::Erase(pTask); //这有可能导致mWaittingNodes中某一Node出现多次，但是是没有问题的
                pTask->SetWaited(nullptr);
            }
        }
        else
        {
            pTask->CompleteDeffered(taskPair.waited);
        }
    };

    auto push2DispatchedQueue = [this](Task* pTask)
    {
        if(pTask->IsSchedulable())
        {
            mReadyTasks.push(pTask);
        }
        else
        {
#ifdef _USING_MULTI_LEVEL_UNSCHEDULABLE
            auto pNode = pTask->GetNode();
            auto& unschedulableList = pNode->GetUnschedulableList(mId);
            if(unschedulableList.Empty())
            {
                mUnschedulableNodes[mUnschedulableNodeCount] = pNode;
                ++mUnschedulableNodeCount;
            }

            unschedulableList.PushBack(pTask);
#else
            pTask->SetWaited(pTask->GetNode());
            pTask->Pending4Lock();
#endif
        }
    };

    while (LIKELY(mIsRuning || mDispatcher->IsRunning() || mFinishedTasks < mNextTaskPos))
    {
        mDispatchedTasks.consume_all(push2DispatchedQueue);
        for(WorkerId fromWorker = 0; fromWorker < mQueueCount; ++fromWorker)
        {
            auto& pTaskQueue = mPendingTasks[fromWorker];
            pTaskQueue.consume_all(push2ReadyQueue);
        }

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
#ifdef _USING_MULTI_LEVEL_UNSCHEDULABLE
    for(auto iNode = 0; iNode < mUnschedulableNodeCount; ++iNode)
    {
        auto pNode = mUnschedulableNodes[iNode];
        auto& unschedulableList = pNode->GetUnschedulableList(mId);
        if(!unschedulableList.Empty())
        {
            auto pTask = unschedulableList.Front();
            if(pTask->IsSchedulable())
            {
                mReadyTasks.push(pTask);
                unschedulableList.PopFront();
            }
        }

        if(!unschedulableList.Empty())
        {
            mUnschedulableNodes[newCount] = pNode;
            ++newCount;
        }
    }

    mUnschedulableNodeCount = newCount;
    newCount = 0;
#endif

    for(auto iNode = 0; iNode < mWaittingNodeCount; ++iNode)
    {
        auto pNode = mWaittingNodes[iNode];
        auto& waittingList = pNode->GetWaittingList(mId);
        if(!waittingList.Empty())
        {
            auto pTask = waittingList.Front();
            bool gotLock = false;
#ifndef _USING_MULTI_LEVEL_UNSCHEDULABLE
            if(pTask->IsSchedulable())
#endif
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
        }

        if(!waittingList.Empty())
        {
            mWaittingNodes[newCount] = pNode;
            ++newCount;
        }
    }

    mWaittingNodeCount = newCount;

    if(UNLIKELY(mOutputWaitingTasks))
    {
        DoOutputWaitingTasks();
    }
}

#include "SharedMutex.h"
void Worker::DoOutputWaitingTasks()
{
    mOutputWaitingTasks = false;
    std::fstream out("Worker" + std::to_string(mId) + ".waiting", std::ios_base::binary|std::ios_base::out);
    for(auto iNode = 0; iNode < mWaittingNodeCount; ++iNode)
    {
        auto pNode = mWaittingNodes[iNode];
        out << "[" << pNode->GetName() << ":" << pNode->GetLastWorkflowId() << "]\n";
        auto& waittingList = pNode->GetWaittingList(mId);
        bool firstWriter = true;
        for(auto pTask = waittingList.Front(); !waittingList.TraverseEnd(pTask); pTask = pTask->Next())
        {
            out << pTask << ":" << pTask->GetName() << ":" << pTask->GetWorkflowId()
                <<":" << pTask->IsAccepted()  << ":" << pTask->GetWaitingLockCount() << "|";
            if(pTask->IsWaittingLock())
            {
                if(firstWriter)
                {
                    firstWriter = false;
                    int32_t readerCount = pNode->GetMutex().GetReaderCount();
                    for(int32_t iTask = 0; iTask <= -readerCount; ++iTask)
                    {
                        auto pSeeTask = pNode->GetMutex().GetWaiters().First(iTask).pTask;
                        out << pSeeTask << ":" << pSeeTask->GetName() << ":" << pSeeTask->GetWorkflowId()
                            << ":" << pSeeTask->IsAccepted()  << ":" << pSeeTask->GetWaitingLockCount() << "|";
                    }
                    out << readerCount;
                }
            }
            else
            {
                out << pTask->GetWaited()->GetName() << ":" << pTask->GetWaited()->GetMutex().GetFirstWaittingWriter();
            }

            out << "\n";
        }
        out << "\n";
    }
    out.close();

    std::fstream outQueue("Worker" + std::to_string(mId) + ".queue", std::ios_base::binary|std::ios_base::out);
    for(auto iNode = 0; iNode < mWaittingNodeCount; ++iNode)
    {
        auto pNode = mWaittingNodes[iNode];
        auto& mutex = pNode->GetMutex();
        auto readers = mutex.GetReaderCount();
        outQueue << "[" << pNode->GetName() << ":" << pNode->GetLastWorkflowId() << (readers>=0?":":":m") << std::abs(readers) << "]\n";

        auto& waiters = mutex.GetWaiters();
        for(int32_t iTask = 0; iTask <= 5 && waiters.Valid(iTask); ++iTask)
        {
            auto pSeeTask = waiters.First(iTask).pTask;
            outQueue << pSeeTask << ":" << pSeeTask->GetName() << ":" << pSeeTask->GetWorkflowId() << ":" << pNode->GetName()
                << ":" << pSeeTask->IsAccepted()  << ":" << pSeeTask->GetWaitingLockCount() << "|\n";
        }

        outQueue << "\n";
    }

    outQueue.close();
}
#else
void Worker::CheckLostLamb( void )  {
    while(!mWaittingTasks.Empty())
    {
        auto pTask = mWaittingTasks.PopFront();
        bool gotLock = false;
#ifndef _USING_MULTI_LEVEL_UNSCHEDULABLE
        if(pTask->IsSchedulable())
#endif
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
        }
        else
        {
            mWaittingTasks.PushBack(pTask);
            break;
        }
    }
}
#endif


void Worker::Stop( void )
{
    mIsRuning = false;
}
