//
// Created by shgli on 17-10-8.
//
#include <string>
#include <thread>
#include <sched.h>
#include "Context.h"
#include "Worker.h"
#include "Dispatcher.h"
#include <boost/filesystem.hpp>
#include "utils/DCallStack.h"
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
#if __APPLE__
#else
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

void SetThreadName(const std::string& name)
{
#if __APPLE__
    pthread_setname_np(name.c_str());
#else
    pthread_setname_np(pthread_self(), name.c_str());
#endif
}

std::unique_ptr<Dispatcher> GlobalDispatcher = nullptr;
thread_local Task* CurrentTask = nullptr;
thread_local Worker* ThisWorker = nullptr;


void Context::SetCurrentTask(Task* pTask)
{
    CurrentTask = pTask;
}

Task* Context::GetCurrentTask( void )
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
static std::atomic<ThreadId> NEXT_MISC_THREAD_ID{-1};
thread_local ThreadId THIS_THREAD_ID = 0;
namespace fs = boost::filesystem;
bool TraverseDirectory(const fs::path& dirPath, std::function<bool (const fs::path& p)> act)
{
    bool hasOne = false;
    fs::directory_iterator end;
    for(fs::directory_iterator pos(dirPath); pos != end; ++pos)
    {
        hasOne = act(*pos) ? true : hasOne;
    }

    return hasOne;
}

void Context::OutputWaittingTasks( void )
{
    for(int32_t workerId = ThreadIndex[ThreadType::WORKER].first; workerId < ThreadIndex[ThreadType::WORKER].second; ++workerId)
    {
        AllWorkers[workerId]->OutputWaitingTasks();
    }
}

void SigUsr1Handler(int32_t)
{
    Context::OutputWaittingTasks();
}
#include <signal.h>
#include <sys/resource.h>
#include <sys/mman.h>
Dispatcher* Context::Init(int32_t workerCount, int32_t miscThreadsNum)
{
    TraverseDirectory("../conf"
            ,[](const auto& p)
                             {
                                 if(p.extension().string() == ".log")
                                 {
                                     LoadLogConf(p.string());
                                     return true;
                                 }
                                 return false;
                             });
    Signal(SIGUSR1, SigUsr1Handler);

    int32_t maxWorkerCount = std::min(std::thread::hardware_concurrency(), static_cast<unsigned int>(AllWorkers.size()));
    if(workerCount > maxWorkerCount)
    {
        throw std::logic_error("max worker count is " + std::to_string(maxWorkerCount));
    }

    {
        struct rlimit rlim = {RLIM_INFINITY, RLIM_INFINITY};
        setrlimit(RLIMIT_MEMLOCK, &rlim);
    }

    if(-1 == mlockall(MCL_CURRENT | MCL_FUTURE))
    {
        std::cout << "count not local memory\n";
        //LOG_WARN(mLog, "count not local memory");
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
        auto& worker = AllWorkers[workerId];
        worker->Start();
    }

    const int32_t cpuid_main = 0;
    Bind2Cpu(cpuid_main);
    SetThreadName("Dispatcher");
    GlobalDispatcher->Run();
    return true;
}

void Context::WaitStart( void )
{
    GlobalDispatcher->WaitStart();
}

void Context::Stop( void )
{
    GlobalDispatcher->Stop();
    GlobalDispatcher->Join();
    int32_t idxWork =  ThreadIndex[ThreadType::WORKER].first;
    while(nullptr != AllWorkers[idxWork])
    {
        AllWorkers[idxWork]->Stop();
        ++idxWork;
    }

    idxWork =  ThreadIndex[ThreadType::WORKER].first;
    while(nullptr != AllWorkers[idxWork])
    {
        WorkerThreads[idxWork]->join();
        ++idxWork;
    }

    idxWork =  ThreadIndex[ThreadType::WORKER].first;
    while(nullptr != AllWorkers[idxWork])
    {
        WorkerThreads[idxWork] = nullptr;
        ++idxWork;
    }

    GlobalDispatcher = nullptr;
    ThreadIndex.clear();
}

void Context::InitMiscThread(const std::string& name)
{
    THIS_THREAD_ID = NEXT_MISC_THREAD_ID.fetch_add(1);
    if(name.empty())
    {
        SetThreadName("Misc-"+std::to_string(THIS_THREAD_ID));
    } else{
        SetThreadName(name);
    }

}

ThreadId Context::GetThreadId( void )
{
    return THIS_THREAD_ID;
}

Logger& Context::GetLog( void )
{
    return ThisWorker->GetLog();
}
