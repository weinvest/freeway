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
            usleep(100);
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
