//
// Created by 李书淦 on 2018/3/25.
//

#ifndef FREEWAY_WORKFLOWCHECKER_H
#define FREEWAY_WORKFLOWCHECKER_H

#include <set>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <boost/test/test_tools.hpp>

class NodeFamilyTree
{
public:
    NodeFamilyTree(int32_t maxNodeCnt)
            :mMaxNodeCnt(maxNodeCnt)
            ,mFamilyTree(new std::set<int32_t>[maxNodeCnt])
    {}

    void AddRelation(int32_t child, int32_t parent) { mFamilyTree[child].insert(parent); }
    void Build( void );
    bool IsAncestor(int32_t child, int32_t parent) { return 0 != mFamilyTree[child].count(parent); }

    int32_t GetMaxNodeCnt( void ) const { return mMaxNodeCnt; }
private:
    int32_t mMaxNodeCnt;
    std::set<int32_t>* mFamilyTree{nullptr};
};

class WorkflowChecker
{
public:
    WorkflowChecker() = default;

    void Initialize(NodeFamilyTree* pFamilyTree);

    NodeFamilyTree& GetFamilyTree() { return *mFamilyTree; }

    void SetObservedValue(int32_t who, int32_t prev, int32_t value)
    {
        mObservedValues[prev * mNodeCount + who] = value;
    }

    void iRun(int32_t who)
    {
        mRunNodes[mIdxRunNode.fetch_add(1)] = who;
    }

    void Check( void );
private:
    bool CanRunBefore(int32_t prev, int32_t succ);

    NodeFamilyTree* mFamilyTree{nullptr};
    int32_t mNodeCount{0};
    int32_t mMaxValueCount{0};
    int32_t *mObservedValues{nullptr};
    int32_t *mRunNodes{nullptr};
    std::atomic_int mIdxRunNode{0};
};;


class WorkflowCheckerPool
{
public:
    WorkflowCheckerPool(NodeFamilyTree& familyTree, int32_t maxWorkflow)
            :mFamilyTree(familyTree)
            ,mCheckers(new WorkflowChecker[maxWorkflow])
            ,mIsWorkflowRun(new bool[maxWorkflow])
            ,mMaxWorkflow(maxWorkflow)
    {
        memset(mIsWorkflowRun, 0, maxWorkflow * sizeof(bool));
        for(auto iWorkflow = 0; iWorkflow < maxWorkflow; ++iWorkflow)
        {
            mCheckers[iWorkflow].Initialize(&mFamilyTree);
        }
    }

    WorkflowChecker* GetChecker(int32_t workflowId);

    int32_t GetMaxWorkflow() const { return mMaxWorkflow; }
    int32_t GetMaxNodeCnt() const { return mFamilyTree.GetMaxNodeCnt(); }
    NodeFamilyTree& GetFamilyTree() const { return mFamilyTree; }

    void CheckAll( void );
private:
    NodeFamilyTree& mFamilyTree;
    WorkflowChecker* mCheckers;
    bool* mIsWorkflowRun;
    int32_t mMaxWorkflow;

};
#endif //FREEWAY_WORKFLOWCHECKER_H
