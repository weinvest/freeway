//
// Created by 李书淦 on 2018/3/25.
//

#ifndef FREEWAY_WORKFLOWCHECKER_H
#define FREEWAY_WORKFLOWCHECKER_H

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
    bool IsAncestor(int3_t child, int32_t parent) { return 0 != mFamilyTree[child].count(parent); }

    int32_t GetMaxNodeCnt( void ) const { return mMaxNodeCnt; }
private:
    int32_t mMaxNodeCnt;
    std::set<int32_t>* mFamilyTree{nullptr};
};

class WorkflowChecker
{
public:
    WorkflowChecker(NodeFamilyTree& familyTree)
            :mFamilyTree(familyTree)
            ,mNodeCount(familyTree.GetMaxNodeCnt())
            ,mMaxValueCount(maxNodeCnt*maxNodeCnt)
            ,mObservedValues(new int32_t[mMaxValueCount])
            ,mRunNodes(new int32_t[maxNodeCnt])
    {
        for(int32_t iValue = 0; iValue < mMaxValueCount; ++iValue)
        {
            mObservedValues[iValue] = -1;
        }

        memset(mRunNodes, 0, sizeof(int32_t)*mNodeCount);
    }


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

    NodeFamilyTree& mFamilyTree;
    int32_t mNodeCount{0};
    int32_t mMaxValueCount{0};
    int32_t *mObservedValues{nullptr};
    int32_t *mRunNodes{nullptr};
    std::atomic_int mIdxRunNode{0};
};;


#endif //FREEWAY_WORKFLOWCHECKER_H
