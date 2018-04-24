//
// Created by shugan.li on 18-3-23.
//

#include "TaskList.h"
#include "Task.h"
TaskList::TaskList(TaskList&& o)
:mHead(o.mHead)
{
    o.mHead = nullptr;
}

Task* TaskList::Pop( void )
{
    auto pTask = mHead;
    mHead = mHead->mNext;
    pTask->mNext = nullptr;
    return pTask;
}

void TaskList::Push(Task* pTask)
{
    if(nullptr == pTask->mNext)
    {
        pTask->mNext = mHead;
        mHead = pTask;
    }
}

void TaskList::Insert(Task* pBefore, Task* pTask)
{
    assert(nullptr == pTask->mNext);
    if(nullptr == pBefore)
    {
        pTask->mNext = mHead;
        mHead = pTask;
    }
    else
    {
        pTask->mNext = pBefore->mNext;
        pBefore->mNext = pTask;
    }
}

void TaskList::Merge(Task* pTail, TaskList& other)
{
    if(nullptr == mHead)
    {
        mHead = other.mHead;
        other.mHead = nullptr;
    }
    else
    {
        pTail->mNext = other.mHead;
        other.mHead = nullptr;
    }
}

bool TaskList::Empty( void ) const
{
    return nullptr == mHead;
}