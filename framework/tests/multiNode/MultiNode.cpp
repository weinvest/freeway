//
// Created by 李书淦 on 2018/3/25.
//
#include <iostream>
#include "MultiNode.h"
int32_t MultiNode::DoProcess(WorkflowID_t workflowId)
{
    if(workflowId <= GetLastWorkflowId())
    {
        std::cout << GetName() <<  " workflow error:" << workflowId <<"<=" << GetLastWorkflowId() << std::endl;
    }
    BOOST_REQUIRE_GT(workflowId, GetLastWorkflowId());
    auto checker = mCheckPool.GetChecker(workflowId-1);

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
    checker->iRun(mId);

    return 0;
}

void MultiNode::AddPrecessor(MultiNode* pParent, bool ignore)
{
    mParents.emplace_back(pParent->Connect(this));
    mIgnoredParent[pParent->GetId()] = ignore;
    if(!ignore) {
        mCheckPool.GetFamilyTree().AddRelation(mId, pParent->GetId());
    }
}


MultiNode* CreateNode(std::vector<MultiNode*>& allNodes, WorkflowCheckerPool& pool, const std::string& nodeName)
{
    static int32_t id = 0;
    static std::default_random_engine generator;
    static std::uniform_int_distribution<int> runCntDist(100, 100000);

    auto pNode = new MultiNode(pool, id++, runCntDist(generator));
    pNode->SetName(nodeName);
    pool.GetFamilyTree().AddNode(pNode);

    allNodes.push_back(pNode);
    return pNode;
}

void FreeNode(std::vector<MultiNode*>& allNodes)
{
    for(auto pNode : allNodes)
    {
        delete pNode;
    }
}

void AddEdge(MultiNode* pChild, MultiNode* pParent)
{
    static std::default_random_engine generator;
    static std::uniform_int_distribution<int> ignoreDist(0, 1);
    pChild->AddPrecessor(pParent, 1 == ignoreDist(generator));
}