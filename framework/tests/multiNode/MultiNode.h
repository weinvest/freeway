//
// Created by 李书淦 on 2018/3/25.
//

#ifndef FREEWAY_MULTINODE_H
#define FREEWAY_MULTINODE_H

#include <cstdint>
#include <boost/test/test_tools.hpp>
#include <clock/Clock.h>
#include "framework/freeway/DEventNode.h"


class MultiNode: public DEventNode
{
public:
    MultiNode(int32_t loopCnt)
            :mLoopCnt(loopCnt)
    {}

    int32_t GetRunCount() const { return mRunCount; }
    void SetRaiseTime(DateTime t) { mRaiseTime = t; }
    TimeSpan GetTotalUsedTime() const { return mUsedTime; }
protected:
    int32_t DoProcess(WorkflowID_t workflowId) override
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

private:
    int32_t mLoopCnt;
    int32_t mRunCount{0};
    DateTime mRaiseTime;
    TimeSpan mUsedTime;
};



#endif //FREEWAY_MULTINODE_H
