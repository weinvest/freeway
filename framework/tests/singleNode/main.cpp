#define BOOST_TEST_MODULE SINGLE_NODE_TEST
#include <thread>
#include <boost/test/included/unit_test.hpp>
#include "framework/freeway/DEventNode.h"
#include "framework/freeway/Context.h"
#include "clock/Clock.h"
#include "testCommon/Delay.h"
class SingleNode: public DEventNode
{
public:
    SingleNode(Delay& delay, int32_t loopCnt)
            :mDelay(delay)
            ,mLoopCnt(loopCnt)
    {}

    int32_t GetRunCount() const { return mRunCount; }
    void SetRaiseTime(DateTime t) { mDelay.setRaisedTime(t); }
protected:
    int32_t DoProcess(WorkflowID_t workflowId) override
    {
//        std::cout << Context::GetWorkerId() << "\n";
        if(workflowId < GetLastWorkflowId())
        {
            int32_t i = 0;
            ++i;
        }
        mDelay.setRunTime(Clock::Instance().Now());
        BOOST_REQUIRE_GT(workflowId, GetLastWorkflowId());
        int32_t i = 0, sum = 0;
        for(; i < mLoopCnt; ++i)
        {
            sum += i;
        }

        ++mRunCount;
#ifdef DEBUG
        std::cout << "workflowId:" << workflowId << ",Workder:" << Context::GetWorkerId() << ", sum:" << sum << ",run:" << mRunCount << "\n";
#endif
        return sum;
    }

private:
    Delay& mDelay;
    int32_t mLoopCnt;
    int32_t mRunCount{0};
};



BOOST_AUTO_TEST_CASE(first_test)
{
    //auto pDispatcher = Context::Init(std::thread::hardware_concurrency()-1, 1);
    auto pDispatcher = Context::Init(1, 1);
    Delay delay(8192);
    auto singleNode = SingleNode(delay, 3000);
    singleNode.SetName("SingleNode");
//    std::cout << (&singleNode) << "\n";
    std::thread t([&singleNode]
                  {
                      Context::InitMiscThread("SingleNode");
                      Context::WaitStart();

                      const int32_t MAX_RUN_COUNT = 100;
                      int32_t runCnt = 0;
                      while(runCnt < MAX_RUN_COUNT)
                      {
                          ++runCnt;
                          singleNode.SetRaiseTime(Clock::Instance().Now());
                          singleNode.RaiseSelf();
                          std::this_thread::sleep_for(std::chrono::microseconds(500));
                      }

                      std::this_thread::sleep_for(std::chrono::microseconds(5000));
                      std::cout << "==================Stopping=============================\n";
                      Context::Stop();
                      BOOST_REQUIRE_GE(singleNode.GetRunCount(), 0);
                      BOOST_REQUIRE_LE(singleNode.GetRunCount(), runCnt);
                  });
    Context::Start();
    if(t.joinable())
    {
        t.join();
    }

    std::cout << "======================================================\n";
    std::cout << "Run " << singleNode.GetRunCount() << " times, Mean frame used time:" << delay.meanTime().total_microseconds() << "\n";
    std::cout << "======================================================\n";
}
