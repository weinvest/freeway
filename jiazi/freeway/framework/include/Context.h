//
// Created by shgli on 17-10-8.
//

#ifndef ARAGOPROJECT_CONTEXT_H
#define ARAGOPROJECT_CONTEXT_H
#include <memory>

class Worker;
class ITask;
class Task;
class Dispatcher;
class SmallObjectAllocatorImpl;

SmallObjectAllocatorImpl* GetAllocator( void );
SmallObjectAllocatorImpl* GetAllocator(int32_t idx);

void SetCurrentTask(ITask* pTask);
void SwitchOut( void );
void EnqueueTask(ITask* pTask);

ITask* GetCurrentTask( void );

Dispatcher* Init(int32_t workerCount, int32_t miscThreadsNum);
Dispatcher* GetDispatcher( void );

Worker* GetWorker( void );
Worker* GetWorker(int32_t idx);
using WorkerId = int32_t;
WorkerId GetWorkerId( void );

bool Start( void );
void Stop( void );


enum class ThreadType
{
    DISPATCHER=0,
    WORKER,
    MISC
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
