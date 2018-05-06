//
// Created by 李书淦 on 2018/4/5.
//
#include "LockPtr.h"
#include "Context.h"
#include "Task.h"
#include "DEventNodeSpecial.h"
#include "DEventNode.h"

void LockPtrBase::WaitSharedLock( void )
{
    auto pThisTask = Context::GetCurrentTask();
    if(mNode != pThisTask->GetNode() && pThisTask != mSpecial->pTask) //同一节点必须保证上一个Workflow与下一个Workflow使用的Task不同
    {
        pThisTask->WaitSharedLock(mNode);

        mSpecial->pTask = pThisTask;
    }
}

