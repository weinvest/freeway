//
// Created by shgli on 17-10-8.
//
#include <string>
#include <thread>
#include <sched.h>
#include "Context.h"
#include "Worker.h"
#include "Dispatcher.h"
#include <stdio.h>

static constexpr int32_t MAX_WORKERS = 64;
/*
thread_local SmallObjectAllocatorImpl* GlobalAllocator = nullptr;
std::array<SmallObjectAllocatorImpl, MAX_WORKERS+1> ALLOCATORS;
SmallObjectAllocatorImpl* GetAllocator( void )
{
    return *GlobalAllocator;
}
*/
#include <iostream>
std::array<int32_t, MAX_WORKERS+1> CPUS;
bool Bind2Cpu(int32_t idxCPU)
{
#if 0
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(idxCPU, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask) != 0)
    {
        std::cout<<"Failed to set affinity-CPU"<<idxCPU<<std::endl;
        return false;
    }
#endif
    return true;
}

std::unique_ptr<Dispatcher> GlobalDispatcher = nullptr;
thread_local ITask* CurrentTask = nullptr;
thread_local Worker* ThisWorker = nullptr;


void SetCurrentTask(ITask* pTask)
{
    CurrentTask = pTask;
}

/*
void SwitchOut( void )
{
    CurrentTask->Suspend();
    CurrentTask = nullptr;
}
*/

void EnqueueTask(ITask* pTask)
{
    pTask->SetReady();
    GetWorker(pTask->GetWorkerId())->Enqueue(GetWorker()->GetId(), pTask);
}


ITask* GetCurrentTask( void )
{
    return CurrentTask;
}

std::array<std::unique_ptr<Worker>, MAX_WORKERS> AllWorkers;
std::array<std::unique_ptr<std::thread>, MAX_WORKERS> WorkerThreads;


std::unordered_map<ThreadType, std::pair<int, int>> ThreadIndex;

    
/*
[0]                                 Dispatch Thread
[1-workCount]                       Worker Thread
[WorkCount+1, WorkCount+Misc+1]     Such As FeedSource/Market/Timer...
*/
Dispatcher* Init(int32_t workerCount, int32_t miscThreadsNum)
{

    int32_t maxWorkerCount = std::min(std::thread::hardware_concurrency(), static_cast<unsigned int>(AllWorkers.size()));
    if(workerCount > maxWorkerCount)
    {
        throw std::logic_error("max worker count is " + std::to_string(maxWorkerCount));
    }

    ThreadIndex.emplace(ThreadType::DISPATCHER, std::make_pair(0,1));
    ThreadIndex.emplace(ThreadType::WORKER, std::make_pair(1,workerCount+1));
    ThreadIndex.emplace(ThreadType::MISC, std::make_pair(workerCount+1,workerCount+miscThreadsNum+1));

//QueueID           0          1.....WorkerCount      WorkerCount+1 ... +MiscThread+1
//Thread        Dispatcher        AllWorkers        FeedSource/Market/Timer

    std::fill(AllWorkers.begin(), AllWorkers.end(), nullptr);
    std::fill(WorkerThreads.begin(), WorkerThreads.end(), nullptr);
    for(int32_t workerId = ThreadIndex[ThreadType::WORKER].first; workerId < ThreadIndex[ThreadType::WORKER].second; ++workerId)
    {
        AllWorkers[workerId].reset(new Worker(workerId, workerCount));
    }

//All of the threads can PUSH node to Dispater's PendingNodesQueue
    GlobalDispatcher.reset(new Dispatcher(workerCount, miscThreadsNum));
    return GlobalDispatcher.get();
}

Dispatcher* GetDispatcher( void )
{
    return GlobalDispatcher.get();
}

Worker* GetWorker( void )
{
    return ThisWorker;
}

Worker* GetWorker(int32_t idx)
{
    return AllWorkers[idx].get();
}

WorkerId GetWorkerId( void )
{
    if(nullptr == ThisWorker)
    {
        return -1;
    }

    return ThisWorker->GetId();
}

bool Start( void )
{
    if(nullptr == GlobalDispatcher)
    {
        return false;
    }

    const int32_t cpuid_main = 0; 
    int32_t idxWork = ThreadIndex[ThreadType::WORKER].first; //WorkerID starts from 1
    while(nullptr != AllWorkers[idxWork])
    {
        WorkerThreads[idxWork] = std::make_unique<std::thread>(std::thread([idxWork]()
                                                       {
                                                            char ThreadName[16];
                                                            
                                                            snprintf(ThreadName, sizeof(ThreadName), "Worker-%02d", idxWork);
                                                            //pthread_setname_np(pthread_self(), ThreadName);
                                                            ThisWorker = AllWorkers[idxWork].get();

                                                            Bind2Cpu(idxWork);
                                                            
                                                            AllWorkers[idxWork]->Run();
                                                            ThisWorker = nullptr;
                                                       }));
        ++idxWork;
    }

    Bind2Cpu(cpuid_main);
    GlobalDispatcher->Run();
    return true;
}

void Stop( void )
{
    int32_t idxWork =  ThreadIndex[ThreadType::WORKER].first;
    while(nullptr != AllWorkers[idxWork])
    {
        AllWorkers[idxWork]->Stop();
        WorkerThreads[idxWork]->join();
        WorkerThreads[idxWork].reset(nullptr);
        ++idxWork;
    }
}
