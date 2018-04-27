//#define BOOST_TEST_MODULE TWO_NODE_TEST
#include <thread>
#include <random>
#include <iostream>
#include "framework/freeway/Context.h"
#include "clock/Clock.h"
#include "../multiNode/MultiNode.h"


//BOOST_AUTO_TEST_CASE(first_test)
int main()
{
    auto pDispatcher = Context::Init(2, 1);

    const int32_t MAX_NODE_COUNT = 32;
    const int32_t MAX_WORKFLOW_COUNT = 1000;

    NodeFamilyTree nodeFamilyTree(MAX_NODE_COUNT);
    WorkflowCheckerPool checker(nodeFamilyTree, 2*MAX_WORKFLOW_COUNT);

    std::vector<MultiNode*> allNodes;
    auto pA = CreateNode(allNodes, checker, "A");
    auto pB = CreateNode(allNodes, checker, "B");

    AddEdge(pB, pA);

    nodeFamilyTree.Build();
    std::thread t ([&allNodes]
                   {
                       Context::InitMiscThread("TwoNode");
                       Context::WaitStart();
                       std::cout << "==================Started=============================\n";

                       std::default_random_engine generator;
                       std::normal_distribution<double> norm(0,1);
                       int32_t runCnt = 0;
                       while(runCnt < MAX_WORKFLOW_COUNT)
                       {
                           ++runCnt;
                           for(auto& pNode : allNodes)
                           {
                               double prob = norm(generator);
                               if(prob > 0.5 || prob < -0.5)
                               {
                                   pNode->RaiseSelf();
                               }
                           }
                           std::this_thread::sleep_for(std::chrono::microseconds(50));
                       }

                       std::this_thread::sleep_for(std::chrono::microseconds(500000));
                       std::cout << "==================Stopping=============================\n";
                       Context::Stop();
                   });
    Context::Start();
    if(t.joinable())
    {
        t.join();
    }

    checker.CheckAll();
    FreeNode(allNodes);
    return 0;
//    auto meanTime = MultiNode.GetTotalUsedTime() / MultiNode.GetRunCount();
//    std::cout << "======================================================\n";
//    std::cout << "Mean frame used time:" << meanTime.total_nanoseconds() << "\n";
//    std::cout << "======================================================\n";
}
