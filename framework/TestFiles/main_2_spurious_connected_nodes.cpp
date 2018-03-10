#include <thread>
#include <unordered_map>
#include <memory>
#include <framework/freeway/DEventNode.h>
#include <framework/freeway/Context.h>
#include <framework/freeway/Worker.h>
#include <framework/freeway/Task.h>
#include <framework/freeway/Dispatcher.h>
#include <functional>
#include <typeinfo>
#include <unistd.h>

//std::unordered_map<DEventNode*, LockPtr<DEventNode>> LockPtrManager;
//std::unordered_map<LockPtr<DEventNode>*, std::unique_ptr<Task>> TaskManager;
//std::vector<std::unique_ptr<Task>> TaskManager;
//std::vector<DEventNode*> NodeManager;

class A1 : public DEventNode{
public:
    A1() { }

    int32_t DoProcess(WorkflowID_t workflowId) 
    {
        char name[16];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        printf("Thread-%s A1::DoProcess flow=%d\n", name, workflowId);
    }

};
class B1 : public DEventNode{
public:
    B1() {    }

    int32_t DoProcess(WorkflowID_t workflowId)
    {
        char name[16];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        printf("Thread-%s Enter B1::DoProcess flow=%d\n", name, workflowId);
        printf("Thread-%s Exit B1::DoProcess flow=%d\n", name, workflowId);
    }
};

class Produce
{
public:
    Produce(int32_t id, DEventNode* pNode):
        mID(id),
        mNode(pNode),
        mIsRunning(true)
        {   }

    void Stop() {mIsRunning = false;}

    void Run() {
        pthread_setname_np(pthread_self(), "Produce");
        while(mIsRunning)
        {
            usleep(1);
            //sleep(1);
            GetDispatcher()->Enqueue(mID, mNode);
        }
    }
private:
    DEventNode* mNode;
    int32_t mID;
    bool mIsRunning;
};

inline void ConnectNodes(DEventNode* pPre, DEventNode* pSuccessor)
{
    pPre->AddSuccessor(pSuccessor);
    pSuccessor->AddPrecursor(pPre);
}

void LinkInfo(DEventNode* pNode)
{
    static WorkflowID_t workFlowID =  0;
    std::cout<<pNode<<": "<<typeid(*pNode).name()<<": "<<std::endl;
    pNode->ShowMutexInfo();
    if(!pNode->GetPrecursors().empty())
    {
        std::cout<<"Precursor:\n";
        for(auto precursor:pNode->GetPrecursors())
            std::cout<<typeid(*precursor).name()<<std::endl;    
    }

    if(!pNode->GetSuccessors().empty())
    {
        std::cout<<"Successor:\n";
        for(auto successor:pNode->GetSuccessors())
            std::cout<<typeid(*successor).name()<<std::endl;
    }

    workFlowID++;
    std::cout<<"=========================================================\n";
}

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
    auto pA1 = new A1();
    pA1->SetName("A1");
    auto pB1 = new B1();
    pB1->SetName("B1");

    ConnectNodes(pA1, pB1);
    LinkInfo(pA1);
    LinkInfo(pB1);


//2. Create Threads

    //Init Queue/AllWorkers[]
    int32_t workCount = std::thread::hardware_concurrency()-1;
    int32_t misc = 2;
    Dispatcher* dispatcher = Init(workCount, misc);

   //0       1,2,3       4,5 
    Produce producer1(std::thread::hardware_concurrency(), pA1);
    std::thread Thread1(std::bind(&Produce::Run, &producer1));

    //Produce producer2(std::thread::hardware_concurrency()+1, pA2);
    //std::thread Thread2(std::bind(&Produce::Run, &producer2));


std::thread Stopper = std::thread(
                        [&producer1]() {
                            pthread_setname_np(pthread_self(), "Stopper");
                            sleep(3600*24);
                            auto cb = std::vector<std::function<void()>>{std::bind(&Dispatcher::Stop, GetDispatcher())};
/*
                            for(auto& worker:AllWorkers)
                            {
                                if(worker)
                                    cb.push_back(std::bind(&Worker::Stop, worker.get()));
                            }
*/
                            cb.push_back(std::bind(&Produce::Stop, &producer1));
                            cb.push_back(Stop);
                            StopAllThreads(cb);
                        }
                      );
    //Threads run()
    std::cout<<Start()<<std::endl;

 //   Thread1.join();
    Thread1.join();
    Stopper.join();

}
