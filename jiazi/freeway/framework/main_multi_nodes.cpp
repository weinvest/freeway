#include <thread>
#include <unordered_map>
#include <memory>
#include <DEventNode.h>
#include <Context.h>
#include <Worker.h>
#include <Task.h>
#include <Dispatcher.h>
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

    void * GetValue(WorkflowID_t flowID)
    {
        LOG_INFO("===========>Enter A1::GetValue  flowID=%d", flowID); 
        sprintf(mValue, "A1::GetValue flow=%d", flowID);
        return  mValue;
    }

    char mValue[64];
};

class A2 : public DEventNode{
public:
    int32_t DoProcess(WorkflowID_t workflowId)
    {
        char name[16];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        printf("Thread-%s A2::DoProcess flow=%d\n", name, workflowId);
    }
};

class A3 : public DEventNode{
public:
    int32_t DoProcess(WorkflowID_t workflowId)
    {
        char name[16];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        printf("Thread-%s A3::DoProcess flow=%d\n", name, workflowId);
    }
};

class B1 : public DEventNode{
public:
    B1(A1* pA1, A2* pA2, A3* pA3) {
             AddPrecursor(pA1);
             AddPrecursor(pA2);
             AddPrecursor(pA3);

            pA1->AddSuccessor(this);
            pA2->AddSuccessor(this);
            pA3->AddSuccessor(this);
    }
    int32_t DoProcess(WorkflowID_t workflowId)
    {
        char name[16];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        printf("Thread-%s B1::DoProcess flow=%d\n", name, workflowId);
        for(auto preNode : mPrecursors)
        {
            RemotePtr<DEventNode> RemoteA1(preNode);
            LOG_INFO("B1 try to call========>%s::GetValue flowID=%d", RemoteA1.get()->GetName().c_str(), workflowId);
            char *pData = static_cast<char*>(RemoteA1->GetValue(workflowId));
            LOG_INFO("Returns From A1: %s", pData);
        }
    }
};

class B2 : public DEventNode{
public:
    B2() {}
    int32_t DoProcess(WorkflowID_t workflowId) 
    {
        char name[16];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        printf("Thread-%s B2::DoProcess flow=%d\n", name, workflowId);
    }
};

class C : public DEventNode {
public:
    C(std::string name): mName(name) {}
    int32_t DoProcess(WorkflowID_t workflowId) 
    {
         char name[16];
         pthread_getname_np(pthread_self(), name, sizeof(name));
         printf("Thread-%s %s::DoProcess flow=%d\n", name, mName.c_str(), workflowId);
    }

private:
    std::string mName;
};


void LinkInfo(DEventNode* pNode)
{
    static WorkflowID_t workFlowID =  0;
    std::cout<<pNode<<": "<<typeid(*pNode).name()<<": "<<std::endl;
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
            //usleep(100);
            sleep(1);
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
    auto pA1 = new A1();
    auto pA2 = new A2();
    auto pA3 = new A3();


    auto pB1 = new B1(pA1, pA2, pA3);


    auto pB2 = new B2();
    ConnectNodes(pA1, pB2);
    ConnectNodes(pA3, pB2);


    auto pC1 = new C("C1");
    ConnectNodes(pB1, pC1);
    ConnectNodes(pB2, pC1);


    auto pC2 = new C("C2");
    ConnectNodes(pB2, pC2);
    
    std::vector<DEventNode*> NodeManager{pA1, pA2, pA3, pB1, pB2, pC1, pC2};
    for(auto pNode:NodeManager)
        LinkInfo(pNode);

//Task Test!!!
    pA1->SetName("A1");
    pA2->SetName("A2");
    pA3->SetName("A3");
    pB1->SetName("B1");
    pB2->SetName("B2");
    pC1->SetName("C1");
    pC2->SetName("C2");



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
