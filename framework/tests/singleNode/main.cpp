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

BOOST_AUTO_TEST_CASE(first_test)
{
    auto pDispatcher = Context::Init(2, 1);

    auto singleNode = SingleNode(3000);
    singleNode.SetName("SingleNode");
//    std::cout << (&singleNode) << "\n";
    std::thread t([&singleNode]
                  {
                      Context::InitMiscThread("SingleNode");
                      Context::WaitStart();

                      const int32_t MAX_RUN_COUNT = 8193;
                      int32_t runCnt = 0;
                      while(runCnt < MAX_RUN_COUNT)
                      {
                          ++runCnt;
                          singleNode.SetRaiseTime(Clock::Instance().Now());
                          singleNode.RaiseSelf();
                          std::this_thread::sleep_for(std::chrono::microseconds(100));
                      }

                      std::this_thread::sleep_for(std::chrono::microseconds(500));
                      Context::Stop();
                      BOOST_CHECK_GE(singleNode.GetRunCount(), 0);
                      BOOST_CHECK_LE(singleNode.GetRunCount(), runCnt);
                  });
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
