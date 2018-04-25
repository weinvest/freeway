//
// Created by shugan.li on 18-3-23.
//

#include "TaskList.h"
#include "Task.h"

TaskList::TaskList()
:mHead(new Task())
{
    mHead->mNext = mHead;
    mHead->mPrev = mHead;
}

TaskList::TaskList(TaskList&& o)
    :mHead(o.mHead)
{
    o.mHead = nullptr;
}

TaskList::~TaskList()
{
    delete mHead;
}

void TaskList::PushFront(Task* pTask)
{
    InsertAfter(mHead, pTask);
}

void TaskList::PushBack(Task *pTask)
{
    InsertBefore(mHead, pTask);
}

Task* TaskList::PopFront( void )
{
    auto pTask = mHead->mNext;
    Erase(pTask);
    return pTask;
}

Task* TaskList::PopBack()
{
    auto pTask = mHead->mPrev;
    Erase(pTask);
    return pTask;
}

void TaskList::Erase(Task* pTask)
{
    pTask->mPrev->mNext = pTask->mNext;
    pTask->mNext->mPrev = pTask->mPrev;
    pTask->mPrev = nullptr;
    pTask->mNext = nullptr;
}

void TaskList::InsertAfter(Task* pPrev, Task* pTask)
{
    pTask->mNext = pPrev->mNext;
    pTask->mPrev = pPrev;
    pPrev->mNext->mPrev = pTask;
    pPrev->mNext = pTask;
}

void TaskList::InsertBefore(Task* pNext, Task* pTask)
{
    pTask->mNext = pNext;
    pTask->mPrev = pNext->mPrev;
    pNext->mPrev->mNext = pTask;
    pNext->mPrev = pTask;
}


bool TaskList::Empty( void ) const
{
    return mHead == mHead->mNext;
}

Task* TaskList::Front( void ) const
{
    return mHead->mNext;
}

Task* TaskList::Back( void )
{
    return mHead->mPrev;
}
