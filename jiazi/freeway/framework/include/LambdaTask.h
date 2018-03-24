//
// Created by shgli on 17-10-10.
//

#ifndef ARAGOPROJECT_LAMBDATASK_H
#define ARAGOPROJECT_LAMBDATASK_H

#include "framework/freeway/ITask.h"
#include "framework/freeway/SmallObjectAllocator.h"
template <typename Func>
class LambdaTask final: public ITask
{
    static constexpr std::string NAME{"Lambda task"};
    Func mFunc;
public:
    LambdaTask(WorkflowId workflowId, const Func& f)
            :ITask(workflowId)
            ,mFunc(f)
    {}

    LambdaTask(WorkflowId workflowId, const Func&& f)
            :ITask(workflowId)
            ,mFunc(std::move(f))
    {}

    void Exec( void ) override { mFunc(); }
    const std::string& GetName( void ) { return NAME; }
};

template <typename Func>
std::shared_ptr<ITask> MakeTask(WorkflowId workflowId, Func&& f)
{
    using T = LambdaTask<Func>;
    return std::allocate_shared<T>(SmallObjectAllocator<T>(), workflowId, std::forward(f));
}

#endif //ARAGOPROJECT_LAMBDATASK_H
