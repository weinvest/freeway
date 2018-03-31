//
// Created by shgli on 17-10-13.
//

#ifndef ARAGOPROJECT_LOCKPTR_H
#define ARAGOPROJECT_LOCKPTR_H

#include "Context.h"
#include "Task.h"
template <typename T>
class LockPtr
{
public:
    LockPtr(T* pNode)
         :mNode(pNode)
         ,mTask(nullptr)
    {}

    ~LockPtr() { mNode = nullptr; }

    T* operator-> ()
    {
        auto pThisTask = Context::GetCurrentTask();
        if(pThisTask != mTask) //同一节点必须保证上一个Workflow与下一个Workflow使用的Task不同
        {
            pThisTask->WaitSharedLock(mNode);
            pThisTask->DecreaseWaitingLockCount();

            mTask = pThisTask;
        }
        return mNode;
    }

    const T*operator->() const
    {
        return const_cast<LockPtr<T>*>(this)->operator->();
    }

    T* get() { return mNode; }

    const T* get() const { return mNode; }
private:
    T* mNode;
    Task* mTask;
};


#endif //ARAGOPROJECT_LOCKPTR_H
