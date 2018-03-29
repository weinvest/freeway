#include <thread>
#include <unordered_map>
#include <pthread.h>
#include <memory>
#include "DEventNode.h"
#include "Context.h"
#include "Worker.h"
#include "Task.h"
#include "Dispatcher.h"
#include <functional>
#include <typeinfo>
#include <unistd.h>
#include <chrono>
#include "clock/Clock.h"

//std::unordered_map<DEventNode*, LockPtr<DEventNode>> LockPtrManager;
//std::unordered_map<LockPtr<DEventNode>*, std::unique_ptr<Task>> TaskManager;
//std::vector<std::unique_ptr<Task>> TaskManager;
//std::vector<DEventNode*> NodeManager;

class A1 : public DEventNode{
public:
    A1(int32_t loopCnt)
            :mLoopCnt(loopCnt)
    {}

    int32_t GetRunCount() const { return mRunCount; }
    void SetRaiseTime(DateTime t) { mRaiseTime = t; }
    TimeSpan GetTotalUsedTime() const { return mUsedTime; }

    int32_t DoProcess(WorkflowID_t workflowId) 
    {
        mUsedTime += (Clock::Instance().Now() - mRaiseTime);
	    int32_t i = 0, sum = 0;
        for(; i < mLoopCnt; ++i)
        {
            sum += i;
        }

        ++mRunCount;
//        std::cout << "workflowId:" << workflowId << ":, sum:" << sum << ",run:" << mRunCount << "\n";
#if 0
        char name[16];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        LOG_INFO("Thread-%s A1::DoProcess flow=%d\n", name, workflowId);
#endif
        return sum;
    }
private:
    int32_t mLoopCnt;
    int32_t mRunCount{0};
    DateTime mRaiseTime;
    TimeSpan mUsedTime;
};


class Produce
{
public:
    Produce(int32_t id, A1* pNode ):
        mID(id),
        mNode(pNode)
        {   }

    void Stop() {}

    void Run() {
        pthread_setname_np("Produce");
        usleep(100);
        const int32_t MAX_RUN_COUNT = 8192;
        int32_t runCnt = 0;
        while(runCnt < MAX_RUN_COUNT )
        {
            ++runCnt;
            mNode->SetRaiseTime(Clock::Instance().Now());
            //sleep(1);

            GetDispatcher()->Enqueue(mID, mNode);
            usleep(50);
        }
    auto meanTime = mNode->GetTotalUsedTime() / mNode->GetRunCount();
    std::cout << "======================================================\n";
    std::cout << "Mean frame used time:" << meanTime.total_microseconds() << "\n";
    std::cout << "======================================================\n";
    }
private:
    A1* mNode;
    int32_t mID;
    bool mIsRunning;
};

inline void ConnectNodes(DEventNode* pPre, DEventNode* pSuccessor)
{
    pPre->AddSuccessor(pSuccessor);
    pSuccessor->AddPrecursor(pPre);
}
/*
inline auto CreateTask(DEventNode* pNode)
{

    auto  iter = LockPtrManager.find(pNode);
    auto pLock = &iter->second;
    auto pair = TaskManager.emplace(pLock, std::make_unique<Task>(-1,-1,&iter->second));
    if(pair.second)
        std::cout<<"Back to main: DEventNode-"<<TaskManager.find(pLock)->second->GetName()<<std::endl;
    else
        std::cout<<"Failed to create Task\n";
        
    return  pair.first;
}
*/
template <typename T>
void StopAllThreads(std::vector<T> StopFunc)
{
    printf("Now, Stop all threads\n");
    for(auto stop:StopFunc)
        stop();
}

extern std::array<std::unique_ptr<Worker>, 64> AllWorkers;
int main()
{

// 1. Create Nodes
    auto pA1 = new A1(1000);
    pA1->SetName("A1");



//2. Create Threads

    //Init Queue/AllWorkers[]
    int32_t workCount = std::thread::hardware_concurrency()-1;
    int32_t misc = 2;
    Dispatcher* dispatcher = Init(workCount, misc);

   //0       1,2,3       4,5 
   // Produce producer1(std::thread::hardware_concurrency(), pB1);
    Produce producer2(std::thread::hardware_concurrency()+1, pA1);
    //std::thread Thread1(std::bind(&Produce::Run, &producer1));
    std::thread Thread2(std::bind(&Produce::Run, &producer2));


std::thread Stopper = std::thread(
                        [&producer2]() {
                            pthread_setname_np("Stopper");
                            sleep(3600*24);
                            auto cb = std::vector<std::function<void()>>{std::bind(&Dispatcher::Stop, GetDispatcher())};
/*
                            for(auto& worker:AllWorkers)
                            {
                                if(worker)
                                    cb.push_back(std::bind(&Worker::Stop, worker.get()));
                            }
*/
                            cb.push_back(std::bind(&Produce::Stop, &producer2));
                            cb.push_back(Stop);
                            StopAllThreads(cb);
                        }
                      );
    //Threads run()
    std::cout<<Start()<<std::endl;

 //   Thread1.join();
    Thread2.join();
    Stopper.join();
   
}
