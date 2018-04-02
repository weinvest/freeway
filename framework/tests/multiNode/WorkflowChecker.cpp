//
// Created by 李书淦 on 2018/3/25.
//
#include <iostream>
#include "WorkflowChecker.h"
#include "MultiNode.h"
void NodeFamilyTree::AddNode(MultiNode* pNode)
{
    mNodes.insert(std::make_pair(pNode->GetId(), pNode));
}

MultiNode* NodeFamilyTree::GetNode(int32_t id)
{
    auto itNode = mNodes.find(id);
    if(mNodes.end() == itNode)
    {
	return nullptr;
    }

    return itNode->second;
}

void NodeFamilyTree:: Build( void )
{
    bool hasChange = true;
    while(hasChange)
    {
        hasChange = false;
        for(int32_t idNode = 0; idNode < mMaxNodeCnt; ++idNode)
        {
            auto& ancestors = mFamilyTree[idNode];
            auto oldSize = ancestors.size();
            std::set<int32_t> merged;
            for(auto idAncestor : ancestors)
            {
                merged.insert(mFamilyTree[idAncestor].begin(), mFamilyTree[idAncestor].end());
            }
            ancestors.insert(merged.begin(), merged.end());
            hasChange = hasChange || oldSize < ancestors.size();
        }
    }
}

void WorkflowChecker::Initialize(NodeFamilyTree* pFamilyTree)
{
    mFamilyTree = pFamilyTree;
    mNodeCount = mFamilyTree->GetMaxNodeCnt();
    mMaxValueCount = mNodeCount*mNodeCount;
    mObservedValues = new int32_t[mMaxValueCount];
    mRunNodes = new int32_t[mNodeCount];

    for(int32_t iValue = 0; iValue < mMaxValueCount; ++iValue)
    {
        mObservedValues[iValue] = -1;
    }

    memset(mRunNodes, 0, sizeof(int32_t)*mNodeCount);
}

bool WorkflowChecker::CanRunBefore(int32_t prev, int32_t succ)
{
    return !mFamilyTree->IsAncestor(prev, succ);
}

void WorkflowChecker::Check(int32_t workflow)
{
    int32_t runNodeCnt = mIdxRunNode.load();
    //check run orders:
    auto prevIdxRun = mRunNodes[0];
    for(auto posRun = 1; posRun < runNodeCnt; ++posRun)
    {
	auto idxRun = mRunNodes[posRun];
        bool canBefore = CanRunBefore(prevIdxRun, idxRun);
        if(!canBefore)
        {
	    std::cout << "ERROR:workflow:" << workflow << ",node:" << mFamilyTree->GetNode(prevIdxRun)->GetName() << " run before node:" << mFamilyTree->GetNode(idxRun)->GetName() << "\n";
        }
        BOOST_REQUIRE(canBefore);
        prevIdxRun = idxRun;
    }

    //check observed values:同一节点的后继在一个workflow中看到的值应该相等
    for(auto idxRun = 0; idxRun < runNodeCnt; ++idxRun)
    {
        auto runNodeId = mRunNodes[idxRun];
        auto thisNode = &mObservedValues[runNodeId*mNodeCount];
        auto observedValue = -1;
        BOOST_CHECK_GE(thisNode[runNodeId], 1);
        for(auto sub = 0; sub < mNodeCount; ++sub)
        {
            if(-1 != thisNode[sub])
            {
                if(-1 == observedValue)
                {
                    observedValue = mObservedValues[sub];
                }
                else
                {
                    BOOST_CHECK_EQUAL(observedValue, thisNode[sub]);
                }

                mObservedValues[sub] = -1;
            }
        }
    }

    mIdxRunNode.store(0);
}


WorkflowChecker* WorkflowCheckerPool::GetChecker(int32_t workflowId)
{
    assert(workflowId <= mMaxWorkflow);
    mIsWorkflowRun[workflowId] = true;
    return &mCheckers[workflowId];
}

void WorkflowCheckerPool::CheckAll( void )
{
    for(int32_t workflow = 0; workflow < mMaxWorkflow; ++workflow)
    {
        if(mIsWorkflowRun[workflow])
        {
            std::cout << "WorkflowId:" << workflow << " has " << mCheckers[workflow].GetRunCount() << " node run\n";
            mCheckers[workflow].Check(workflow);
        }
    }
}
