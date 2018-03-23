#define BOOST_TEST_MODULE SINGLE_NODE_TEST
#include <thread>
#include <boost/test/included/unit_test.hpp>
#include "framework/freeway/DEventNode.h"
#include "framework/freeway/Context.h"
#include "clock/Clock.h"
class SingleNode: public DEventNode
{
public:
    SingleNode(int32_t loopCnt)
            :mLoopCnt(loopCnt)
    {}

    int32_t GetRunCount() const { return mRunCount; }
    void SetRaiseTime(DateTime t) { mRaiseTime = t; }
    TimeSpan GetTotalUsedTime() const { return mUsedTime; }
protected:
    int32_t DoProcess(WorkflowID_t workflowId) override
    {
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

BOOST_AUTO_TEST_CASE(first_test)
{
    auto pDispatcher = Context::Init(2, 1);

    auto singleNode = SingleNode(3000);
    std::thread t = std::move(Context::StartMiscThread([&singleNode]
                  {
WaitStart();
                      int32_t runCnt = 0;
while(runCnt < 100)
                      {
                          ++runCnt;
                          singleNode.SetRaiseTime(Clock::Instance().Now());
                          singleNode.RaiseSelf();
                          std::this_thread::sleep_for(std::chrono::microseconds(100));
                      }

                      Context::Stop();
                      BOOST_CHECK_EQUAL(singleNode.GetRunCount(), runCnt);
                  }));
    Context::Start();
    if(t.joinable())
    {
        t.join();
    }

    auto meanTime = singleNode.GetTotalUsedTime() / singleNode.GetRunCount();
    std::cout << "======================================================\n";
    std::cout << "Mean frame used time:" << meanTime.total_microseconds() << "\n";
    std::cout << "======================================================\n";
}
