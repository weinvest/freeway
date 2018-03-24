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
//    cpu_set_t mask;
//    CPU_ZERO(&mask);
//    CPU_SET(idxCPU, &mask);
//    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask) != 0)
//    {
//        std::cout<<"Failed to set affinity-CPU"<<idxCPU<<std::endl;
//        return false;
//    }
    return true;
}

void SetThreadName(const std::string& name)
{
    pthread_setname_np(name.c_str());
}

std::unique_ptr<Dispatcher> GlobalDispatcher = nullptr;
thread_local Task* CurrentTask = nullptr;
thread_local Worker* ThisWorker = nullptr;


void Context::SetCurrentTask(Task* pTask)
{
    CurrentTask = pTask;
}

void Context::SwitchOut( void )
{
    CurrentTask->Suspend();
    ThisWorker->Push2WaittingList(CurrentTask);
    CurrentTask = nullptr;
}

Task* Context::GetCurrentTask( void )
{
    return CurrentTask;
}

void Context::Enqueue(int32_t from, void* pWho, Task* pTask)
{
    pTask->GetWorker()->Enqueue(from, pWho, pTask);
}

std::array<std::unique_ptr<Worker>, MAX_WORKERS> AllWorkers;
std::array<std::unique_ptr<std::thread>, MAX_WORKERS> WorkerThreads;


std::unordered_map<ThreadType, std::pair<int, int>> ThreadIndex;

    
/*
[0]                                 Dispatch Thread
[1-workCount]                       Worker Thread
[WorkCount+1, WorkCount+Misc+1]     Such As FeedSource/Market/Timer...
*/
static std::atomic<ThreadId> NEXT_MISC_THREAD_ID{-1};
thread_local ThreadId THIS_THREAD_ID = 0;

Dispatcher* Context::Init(int32_t workerCount, int32_t miscThreadsNum)
{

    int32_t maxWorkerCount = std::min(std::thread::hardware_concurrency(), static_cast<unsigned int>(AllWorkers.size()));
    if(workerCount > maxWorkerCount)
    {
        throw std::logic_error("max worker count is " + std::to_string(maxWorkerCount));
    }

    ThreadIndex.emplace(ThreadType::DISPATCHER, std::make_pair(0,1));
    ThreadIndex.emplace(ThreadType::WORKER, std::make_pair(1,workerCount+1));
    ThreadIndex.emplace(ThreadType::MISC, std::make_pair(workerCount+1,workerCount+miscThreadsNum+1));
    NEXT_MISC_THREAD_ID = workerCount+1;
//QueueID           0          1.....WorkerCount      WorkerCount+1 ... +MiscThread+1
//Thread        Dispatcher        AllWorkers        FeedSource/Market/Timer

    std::fill(AllWorkers.begin(), AllWorkers.end(), nullptr);
    std::fill(WorkerThreads.begin(), WorkerThreads.end(), nullptr);

    GlobalDispatcher.reset(new Dispatcher(workerCount, miscThreadsNum));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8388608);

    for(int32_t workerId = ThreadIndex[ThreadType::WORKER].first; workerId < ThreadIndex[ThreadType::WORKER].second; ++workerId)
    {
        AllWorkers[workerId].reset(new Worker(GlobalDispatcher.get(), workerId, workerCount));
        AllWorkers[workerId]->Initialize();
        WorkerThreads[workerId] = std::make_unique<std::thread>(std::thread([workerId]()
                                                                           {

                                                                               SetThreadName("Worker-" + std::to_string(workerId));
                                                                               ThisWorker = AllWorkers[workerId].get();
                                                                               THIS_THREAD_ID = workerId;
                                                                               Bind2Cpu(workerId);
//                                                                               ThisWorker->Initialize();

                                                                               ThisWorker->WaitStart();
                                                                               ThisWorker->Run();
                                                                               ThisWorker = nullptr;
//                                                                               printf("worker's stack size:%zu\n", pthread_get_stacksize_np(pthread_self()));
                                                                           }));
    }
//    printf("dispatcher's stack size:%zu\n", pthread_get_stacksize_np(pthread_self()));
    //All of the threads can PUSH node to Dispater's PendingNodesQueue
    return GlobalDispatcher.get();
}

Dispatcher* Context::GetDispatcher( void )
{
    return GlobalDispatcher.get();
}

Worker* Context::GetWorker( void )
{
    return ThisWorker;
}

Worker* Context::GetWorker(int32_t idx)
{
    return AllWorkers[idx].get();
}

WorkerId Context::GetWorkerId( void )
{
    if(nullptr == ThisWorker)
    {
        return -1;
    }

    return ThisWorker->GetId();
}


bool Context::Start( void )
{
    if(nullptr == GlobalDispatcher)
    {
        return false;
    }

    for(int32_t workerId = ThreadIndex[ThreadType::WORKER].first; workerId < ThreadIndex[ThreadType::WORKER].second; ++workerId)
    {
        auto& pTask = AllWorkers[workerId];
        while(!pTask->IsInitialized());
        pTask->Start();
    }

    const int32_t cpuid_main = 0;
    Bind2Cpu(cpuid_main);
    SetThreadName("Dispatcher");
    GlobalDispatcher->Run();
    return true;
}

void Context::WaitStart( void )
{
    while(!GlobalDispatcher->IsRunning())
    {}
}

void Context::Stop( void )
{
    GlobalDispatcher->Stop();
    int32_t idxWork =  ThreadIndex[ThreadType::WORKER].first;
    while(nullptr != AllWorkers[idxWork])
    {
        AllWorkers[idxWork]->Stop();
        WorkerThreads[idxWork]->join();
        WorkerThreads[idxWork].reset(nullptr);
        ++idxWork;
    }
}

std::thread Context::StartMiscThread(std::function<void()> f)
{
    return std::move(std::thread([f](){ THIS_THREAD_ID = NEXT_MISC_THREAD_ID.fetch_add(1); f(); }));
}

ThreadId Context::GetThreadId( void )
{
    return THIS_THREAD_ID;
}
