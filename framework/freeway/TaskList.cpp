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

bool TaskList::Empty( void ) const
{
    return nullptr == mHead;
}