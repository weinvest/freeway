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
    std::ofstream curGraph("cur.dot", std::ios_base::binary | std::ios_base::out);
    curGraph << "digraph g{\n";
    for(int32_t idNode = 0; idNode < mMaxNodeCnt; ++idNode)
    {
        auto& ancestors = mFamilyTree[idNode];
        for(auto idAncestor : ancestors)
        {
            curGraph << char(idAncestor+'A') << "->" << char(idNode+'A') << ";\n";
        }
    }

    curGraph << "}\n";
    curGraph.close();
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

WorkflowChecker::~WorkflowChecker()
{
    delete[] mObservedValues;
    delete[] mRunNodes;
}

void WorkflowChecker::Initialize(NodeFamilyTree* pFamilyTree)
{
    mFamilyTree = pFamilyTree;
    mNodeCount = mFamilyTree->GetMaxNodeCnt();
    mMaxValueCount = mNodeCount*mNodeCount;
    mObservedValues = new ObservedValue[mMaxValueCount];
    mRunNodes = new RunNode[mNodeCount];

//    std::fill(mObservedValues, mObservedValues+mMaxValueCount, -1);
//    std::fill(mRunNodes, mRunNodes+mNodeCount, -1);
}

bool WorkflowChecker::CanRunBefore(int32_t prev, int32_t succ)
{
    return !mFamilyTree->IsAncestor(prev, succ);
}

void WorkflowChecker::Check(int32_t workflow)
{
    int32_t runNodeCnt = mIdxRunNode.load();
    //check run orders:
    for(auto oPos = 0; oPos < runNodeCnt-1; ++oPos) {
        auto prevRun = mRunNodes[oPos];
//        std::cout << "--RunNode:" << char('A' + prevRun.id) << "---\n";

        for (auto posRun = oPos + 1; posRun < runNodeCnt; ++posRun) {
            auto idxRun = mRunNodes[posRun];
            bool canBefore = CanRunBefore(prevRun.id, idxRun.id);
            if (!canBefore) {
                std::cout << "ERROR:workflow:" << workflow << ",node:[" << mFamilyTree->GetNode(prevRun.id)->GetName()
                          << "," << prevRun.workflow << "] run before node:[" << mFamilyTree->GetNode(idxRun.id)->GetName()
                          << "," << idxRun.workflow << "]\n";
            }
            BOOST_CHECK(canBefore);
        }
    }

    //check observed values:同一节点的后继在一个workflow中看到的值应该相等
    for(auto idxRun = 0; idxRun < runNodeCnt; ++idxRun)
    {
        auto runNodeId = mRunNodes[idxRun].id;
        auto runNodeName = mFamilyTree->GetNode(runNodeId)->GetName();
        auto thisNode = &mObservedValues[runNodeId*mNodeCount];
        auto observedValue = thisNode[runNodeId];
        BOOST_CHECK_GE(observedValue.value, 1);
        BOOST_CHECK_EQUAL(observedValue.workflow, workflow);
        for(auto sub = 0; sub < mNodeCount; ++sub)
        {
            if(-1 != thisNode[sub].workflow)
            {
                if(observedValue != thisNode[sub]) {
                    auto subNodeName = mFamilyTree->GetNode(sub)->GetName();
                    std::cout << "ERROR:workflow:" << workflow << ",node:" << subNodeName
                              << " see node: " << runNodeName << " failed:" << subNodeName << "[" << thisNode[sub].workflow << ":" << thisNode[sub].value
                              << "]!=" << runNodeName << "[" << observedValue.workflow << ":" << observedValue.value << "]\n";

                    BOOST_CHECK_EQUAL(observedValue.workflow, thisNode[sub].workflow);
                    BOOST_CHECK_EQUAL(observedValue.value, thisNode[sub].value);
                }
                thisNode[sub] = ObservedValue{-1, -1};
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
