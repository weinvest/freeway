//
// Created by 李书淦 on 2018/3/25.
//

#include "MultiNode.h"
int32_t MultiNode::DoProcess(WorkflowID_t workflowId) override
{
//        std::cout << Context::GetWorkerId() << "\n";
    mUsedTime += (Clock::Instance().Now() - mRaiseTime);
    BOOST_CHECK_GT(workflowId, GetLastWorkflowId());
    int32_t i = 0, sum = 0;
    for(; i < mLoopCnt; ++i)
    {
        sum += i;
    }

    ++mRunCount;
//        std::cout << "workflowId:" << workflowId << ":, sum:" << sum << ",run:" << mRunCount << "\n";
    return sum;
}

void MultiNode::AddPrecessor(MultiNode* pParent)
{
    pParent->Connect(this);
    mFamilyTree.AddRelation(mId, pParent->GetId());
}