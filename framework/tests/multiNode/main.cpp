#define BOOST_TEST_MODULE SINGLE_NODE_TEST
#include <thread>
#include <boost/test/included/unit_test.hpp>
#include "framework/freeway/Context.h"
#include "clock/Clock.h"
#include "MultiNode.h"

BOOST_AUTO_TEST_CASE(first_test)
{
    auto pDispatcher = Context::Init(2, 1);

    auto MultiNode = MultiNode(3000);
    MultiNode.SetName("MultiNode");
//    std::cout << (&MultiNode) << "\n";
    std::thread t = std::move(Context::StartMiscThread([&MultiNode]
                  {
                      Context::WaitStart();

                      const int32_t MAX_RUN_COUNT = 8193;
                      int32_t runCnt = 0;
                      while(runCnt < MAX_RUN_COUNT)
                      {
                          ++runCnt;
                          MultiNode.SetRaiseTime(Clock::Instance().Now());
                          MultiNode.RaiseSelf();
                          std::this_thread::sleep_for(std::chrono::microseconds(50));
                      }

                      Context::Stop();
                      BOOST_CHECK_GE(MultiNode.GetRunCount(), 0);
                      BOOST_CHECK_LE(MultiNode.GetRunCount(), runCnt);
                  }));
    Context::Start();
    if(t.joinable())
    {
        t.join();
    }

    auto meanTime = MultiNode.GetTotalUsedTime() / MultiNode.GetRunCount();
    std::cout << "======================================================\n";
    std::cout << "Mean frame used time:" << meanTime.total_nanoseconds() << "\n";
    std::cout << "======================================================\n";
}
