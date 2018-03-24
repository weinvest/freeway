//
// Created by shgli on 17-10-8.
//

#ifndef ARAGOPROJECT_CONTEXT_H
#define ARAGOPROJECT_CONTEXT_H
#include <memory>
#include <functional>
#include <thread>
class Worker;
class Task;
class Task;
class Dispatcher;
class SmallObjectAllocatorImpl;

enum class ThreadType
{
    DISPATCHER=0,
    WORKER,
    MISC
};

using WorkerId = int32_t;
using ThreadId = int32_t;
class Context {
public:
    static SmallObjectAllocatorImpl *GetAllocator(void);

    static SmallObjectAllocatorImpl *GetAllocator(int32_t idx);

    static void SetCurrentTask(Task *pTask);

    static void SwitchOut(void);

    static Task *GetCurrentTask(void);

    static Dispatcher *Init(int32_t workerCount, int32_t miscThreadsNum);

    static Dispatcher *GetDispatcher(void);

    static void Enqueue(int32_t from, void *pWho, Task *pTask);

    static Worker *GetWorker(void);

    static Worker *GetWorker(int32_t idx);

    static WorkerId GetWorkerId(void);

    static bool Start(void);
    static void WaitStart( void );

    static void Stop(void);

    static std::thread StartMiscThread(std::function<void()> f);

    static ThreadId GetThreadId( void );
};

namespace std
{
    template<> 
    struct hash<ThreadType>
    {
        typedef ThreadType argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& s) const noexcept
        {
            return std::hash<std::size_t>()(static_cast<std::size_t>(s));
        }
    };
}


#endif //ARAGOPROJECT_CONTEXT_H
