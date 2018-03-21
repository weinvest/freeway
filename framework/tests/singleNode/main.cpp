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

        std::cout << workflowId << ":" << sum << "\n";
        return sum;
    }

private:
    int32_t mLoopCnt;
};

BOOST_AUTO_TEST_CASE(first_test)
{
    auto pDispatcher = Context::Init(2, 1);

    auto pSingleNode = SingleNode(3000);
    std::thread t = std::move(Context::StartMiscThread([&pSingleNode]
                  {
                      pSingleNode.RaiseSelf();

                      std::this_thread::sleep_for(std::chrono::milliseconds(50));
                  }));
    Context::Start();

}