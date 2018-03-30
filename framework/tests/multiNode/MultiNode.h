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
#include "framework/freeway/LockPtr.h"
class MultiNode: public DEventNode
{
public:
    MultiNode(WorkflowCheckerPool& checkPool, int32_t id, int32_t loopCnt)
            :mCheckPool(checkPool)
            ,mId(id)
            ,mLoopCnt(loopCnt)
            ,mIgnoredParent(new int32_t[checkPool.GetMaxNodeCnt()])
    {
        assert(loopCnt >= 100);
    }

    int32_t GetId() const { return mId; }

    //void SetRaiseTime(DateTime t) { mRaiseTime = t; }
    //TimeSpan GetTotalUsedTime() const { return mUsedTime; }

    void AddPrecessor(MultiNode* pParent, bool ignore);

    int32_t GetValue( void ) const { return mValue; }
protected:
    int32_t DoProcess(WorkflowID_t workflowId) override;

    WorkflowCheckerPool& mCheckPool;

    int32_t mId;
    int32_t mValue{0};
    int32_t mLoopCnt;
    int32_t *mIgnoredParent;
    std::vector<LockPtr<MultiNode>> mParents;
};



#endif //FREEWAY_MULTINODE_H
