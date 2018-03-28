//
// Created by 李书淦 on 2018/3/25.
//

#include "MultiNode.h"
int32_t MultiNode::DoProcess(WorkflowID_t workflowId)
{
    BOOST_CHECK_GT(workflowId, GetLastWorkflowId());
    auto checker = mCheckPool.GetChecker(workflowId);
    checker->iRun(mId);

    //compute
    int32_t sum = 0;
    for(auto i = 0; i < mLoopCnt; ++i)
    {
        sum += i+1;
    }

    mValue += (sum * 2) / ((1 + mLoopCnt) * mLoopCnt);
    checker->SetObservedValue(mId, mId, mValue);
    for(auto pParent : mParents)
    {
        auto parentId = pParent.get()->GetId();
        if(!mIgnoredParent[parentId])
        {
            checker->SetObservedValue(mId, parentId, pParent->GetValue());
        }
    }

    return 0;
}

void MultiNode::AddPrecessor(MultiNode* pParent, bool ignore)
{
    pParent->Connect(this);
    mIgnoredParent[pParent->GetId()] = ignore;
    mCheckPool.GetFamilyTree().AddRelation(mId, pParent->GetId());
    mParents.push_back(LockPtr<MultiNode>(pParent));
}
