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

    int32_t GetRunCount() const { return mRunCount; }
protected:
    int32_t DoProcess(WorkflowID_t workflowId) override
    {
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
};

BOOST_AUTO_TEST_CASE(first_test)
{
    auto pDispatcher = Context::Init(2, 1);

    auto singleNode = SingleNode(3000);
    std::thread t = std::move(Context::StartMiscThread([&singleNode]
                  {
                      int32_t runCnt = 0;
                      while(runCnt < 100)
                      {
                          ++runCnt;
                          singleNode.RaiseSelf();
                          std::this_thread::sleep_for(std::chrono::milliseconds(50));
                      }

                      Context::Stop();
                      BOOST_CHECK_EQUAL(singleNode.GetRunCount(), runCnt);
                  }));
    Context::Start();
    if(t.joinable())
    {
        t.join();
    }
}