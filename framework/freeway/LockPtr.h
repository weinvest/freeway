//
// Created by shgli on 17-10-13.
//

#ifndef ARAGOPROJECT_LOCKPTR_H
#define ARAGOPROJECT_LOCKPTR_H

#include "Context.h"
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
        return static_cast<T*>(const_cast<LockPtr<T>*>(this)->operator->());
    }

    const T*operator->() const
    {
        auto pThisTask = GetCurrentTask();
        if(!mNode->HasSharedLock4(pThisTask))
        {
            pThisTask->SetWaited(mNode);
            SwitchOut();
            pThisTask->SetWaited(nullptr);
        }

        if(pThisTask != mTask) //同一节点必须保证上一个Workflow与下一个Workflow使用的Task不同
        {
            mTask = pThisTask;
            pThisTask->DecreaseWaitingLockCount();
        }
        return mNode;
    }



private:
    T* mNode;
    ITask* mTask;
};


#endif //ARAGOPROJECT_LOCKPTR_H
