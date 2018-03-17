//
// Created by shugan.li on 18-3-8.
//

#include "ITask.h"
#include "DEventNode.h"
void ITask::Update(WorkflowID_t flow, Worker* worker, DEventNode* pNode)
{
    mWorkflowId = flow;
    mWorker = worker;
    mNodePtr = pNode;
    mWaitingLockCount = pNode->GetPrecursors().size();
    mLevel = 0;
}
