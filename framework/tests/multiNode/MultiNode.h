//
// Created by 李书淦 on 2018/3/25.
//

#ifndef FREEWAY_MULTINODE_H
#define FREEWAY_MULTINODE_H

#include <cstdint>
#include <boost/test/test_tools.hpp>
#include <clock/Clock.h>
#include "framework/freeway/DEventNode.h"
#include "WorkflowChecker.h"

class MultiNode: public DEventNode
{
public:
    MultiNode(WorkflowChecker& familyTree, int32_t id, int32_t loopCnt)
            :mFamilyTree(familyTree)
            ,mId(id)
            ,mLoopCnt(loopCnt)
    {}

    int32_t GetId() const { return mId; }

    int32_t GetRunCount() const { return mRunCount; }
    void SetRaiseTime(DateTime t) { mRaiseTime = t; }
    TimeSpan GetTotalUsedTime() const { return mUsedTime; }

    void AddPrecessor(MultiNode* pParent);
protected:
    int32_t DoProcess(WorkflowID_t workflowId) override;

    WorkflowChecker& mFamilyTree;

    int32_t mId;
};



#endif //FREEWAY_MULTINODE_H
