//
// Created by shgli on 17-9-27.
//
#include "Task.h"
#include "Dispatcher.h"
#include "Context.h"
#include "DEventNode.h"
#include "Worker.h"
#include "SharedMutex.h"

const int32_t capacity = 1024 * 1024;

extern std::unordered_map<ThreadType, std::pair<int, int>> ThreadIndex;
static WorkflowID_t workflowId = 0;
static bool isFirstNode = true;

Dispatcher::Dispatcher(int32_t workerCount, int32_t miscThread)
        : mWorkerCount(workerCount), mMiscThreadCount(miscThread),
          mQueueNum(workerCount + miscThread + 1)  //[0...mQueueNum)
        , mPendingNodes(new PendingNodeQueue[mQueueNum]), mIsRunning(true) {
    for (int i = 0; i < mQueueNum; i++) {
        mPendingNodes[i].Init(capacity, new DummyNode());
    }

    mPendingTask.reserve(128);
}

bool Dispatcher::Enqueue(int32_t fromID, DEventNode *pNode) {
    if (mIsRunning) {
        auto &pNodeQueue = mPendingNodes[fromID];
        pNodeQueue.Push(pNode);
        return true;
    }
    return false;
}

Dispatcher::~Dispatcher() {
    delete[] mPendingNodes;
}

WorkerID_t Dispatcher::SelectWorker(DEventNode *pNode) {
    //---jiazi  Select the minimum overhead and cache-friend worker?
    static size_t Loop = 0;
    Loop = Loop % mWorkerCount;
    return ++Loop;
}

ITask *Dispatcher::VisitNode(DEventNode *pNode, int32_t level) {
    static int DispatchIndex = ThreadIndex[ThreadType::DISPATCHER].first;
    if (isFirstNode) {
        isFirstNode = false;
        ++workflowId;
    }

    //一个节点有多个前驱节点时，只能通过一个前驱节点被Dispatch。
    if (!pNode->HasScheduled(workflowId)) {
        pNode->SetDispatchedID(workflowId);
        //1. select worker
        //2. create task
        //3. acquire locks (Before Worker::Enqueue)
        //4. visit children
        auto idxWorker = SelectWorker(pNode);

        auto pTargetWorker = GetWorker(idxWorker);
        ITask *pTask = pTargetWorker->AllocateTaskFromPool(workflowId, pTargetWorker, pNode);
        pTask->SetLevel(level);
        mPendingTask.push_back(pTask);

        auto &precessor = pNode->GetPrecursors();
        for (auto pPrecessor : precessor) {
            pPrecessor->GetMutex().LockShared(pTask);
        }
        pNode->GetMutex().Lock(pTask);
        ::Enqueue(DispatchIndex, nullptr, pTask);

        auto &successors = pNode->GetSuccessors();
        for (auto pSuccessor : successors) {
            VisitNode(pSuccessor, level + 1);
        }

        return pTask;
    }
    //LOG_ERROR("Node-%s has been scheduled!!!!\n", pNode->GetName().c_str());
    return nullptr;
};


#include <stdio.h>

void Dispatcher::Run(void) {
//    pthread_setname_np(pthread_self(), "Dispatcher");

    while (mIsRunning) {
        static int DispatchIndex = ThreadIndex[ThreadType::DISPATCHER].first;
        //Does dispatcher dispatch the task to itself???
        mPendingTask.clear();
        for (WorkerId fromWorker = DispatchIndex; fromWorker < mQueueNum; ++fromWorker) {
            auto &pNodeQueue = mPendingNodes[fromWorker];
            // printf("Dispatcher consumes Queue-%d\n", fromWorker);
            pNodeQueue.consume_all(std::bind(&Dispatcher::VisitNode, this, std::placeholders::_1, 0));
        }

        isFirstNode = true;
    }
}
