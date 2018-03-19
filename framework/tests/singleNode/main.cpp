#define BOOST_TEST_MODULE SINGLE_NODE_TEST
#include <boost/test/included/unit_test.hpp>

#include "framework/freeway/DEventNode.h"
#include "framework/freeway/Context.h"
#include <thread>
class SingleNode: public DEventNode
{
public:
    SingleNode(int32_t loopCnt)
            :mLoopCnt(loopCnt)
    {}

protected:
    int32_t DoProcess(WorkflowID_t workflowId) override
    {
        int32_t i = 0, sum = 0;
        for(; i < mLoopCnt; ++i)
        {
            sum += i;
        }

        return sum;
    }

private:
    int32_t mLoopCnt;
};

BOOST_AUTO_TEST_CASE(first_test)
{
    auto pDispatcher = Init(3, 0);

    auto pSingleNode = SingleNode(3000);
    std::thread t([pSingleNode]
                  {
                      pSingleNode->RaiseSelf();
                  })
    Start();

}