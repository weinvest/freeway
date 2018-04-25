//#define BOOST_TEST_MODULE MULTI_NODE_TEST
#include <thread>
#include <random>
#include <iostream>
//#include <boost/test/included/unit_test.hpp>
#include "framework/freeway/Context.h"
#include "clock/Clock.h"
#include "MultiNode.h"


//BOOST_AUTO_TEST_CASE(first_test)
int main( void )
{
    auto pDispatcher = Context::Init(3, 1);

    const int32_t MAX_NODE_COUNT = 32;
    const int32_t MAX_WORKFLOW_COUNT = 500;

    NodeFamilyTree nodeFamilyTree(MAX_NODE_COUNT);
    WorkflowCheckerPool checker(nodeFamilyTree, 20*MAX_WORKFLOW_COUNT);

    std::vector<MultiNode*> allNodes;
    auto pA = CreateNode(allNodes, checker, "A");
    auto pB = CreateNode(allNodes, checker, "B");
    auto pC = CreateNode(allNodes, checker, "C");
    auto pD = CreateNode(allNodes, checker, "D");
    auto pE = CreateNode(allNodes, checker, "E");
    auto pF = CreateNode(allNodes, checker, "F");
    auto pG = CreateNode(allNodes, checker, "G");
    auto pH = CreateNode(allNodes, checker, "H");
    auto pI = CreateNode(allNodes, checker, "I");
    auto pJ = CreateNode(allNodes, checker, "J");
    auto pK = CreateNode(allNodes, checker, "K");
    auto pL = CreateNode(allNodes, checker, "L");
    auto pM = CreateNode(allNodes, checker, "M");
    auto pN = CreateNode(allNodes, checker, "N");
    auto pO = CreateNode(allNodes, checker, "O");
    auto pP = CreateNode(allNodes, checker, "P");
    auto pQ = CreateNode(allNodes, checker, "Q");
    auto pR = CreateNode(allNodes, checker, "R");
    auto pS = CreateNode(allNodes, checker, "S");
    auto pT = CreateNode(allNodes, checker, "T");

    AddEdge(pB, pA);
    AddEdge(pB, pC);
    AddEdge(pD, pB);
    AddEdge(pE, pC);
    AddEdge(pF, pB);
    AddEdge(pF, pE);
    AddEdge(pH, pG);
    AddEdge(pH, pD);
    AddEdge(pI, pH);
    AddEdge(pJ, pI);
    AddEdge(pK, pI);
    AddEdge(pL, pD);
    AddEdge(pO, pF);
    AddEdge(pO, pN);
    AddEdge(pP, pF);
    AddEdge(pQ, pM);
    AddEdge(pS, pR);
    AddEdge(pS, pP);
    AddEdge(pT, pN);

    nodeFamilyTree.Build();
    std::thread t ([&allNodes]
                  {
                      try {
                          Context::InitMiscThread("MultiNode");
                          Context::WaitStart();
                          std::cout << "==================Started=============================\n";

                          std::default_random_engine generator;
                          std::normal_distribution<double> norm(0, 1);
                          int32_t runCnt = 0;
                          while (runCnt < MAX_WORKFLOW_COUNT) {
                              ++runCnt;
                              for (auto &pNode : allNodes) {
                                  double prob = norm(generator);
                                  if (prob > 0.5 || prob < -0.5) {
                                      pNode->RaiseSelf();
                                  }
                              }
                              std::this_thread::sleep_for(std::chrono::microseconds(50));
                          }

                          std::this_thread::sleep_for(std::chrono::microseconds(500000));
                          std::cout << "==================Stopping=============================\n";
                          Context::Stop();
                      }
                      catch (const std::exception& ex)
                      {
                          std::cout << "Misc exception:" << ex.what() << "\n";
                      }
                  });

    try
    {
        Context::Start();
        if(t.joinable())
        {
            t.join();
        }

        checker.CheckAll();
        FreeNode(allNodes);
    }
    catch(const std::exception& ex)
    {
        std::cout << "Main exception:" << ex.what() << "\n";
        return 1;
    }
    return 0;
//    auto meanTime = MultiNode.GetTotalUsedTime() / MultiNode.GetRunCount();
//    std::cout << "======================================================\n";
//    std::cout << "Mean frame used time:" << meanTime.total_nanoseconds() << "\n";
//    std::cout << "======================================================\n";
}
