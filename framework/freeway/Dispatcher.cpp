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

Dispatcher::Dispatcher(int32_t workerCount, int32_t miscThread)
        : mWorkerCount(workerCount), mMiscThreadCount(miscThread),
          mQueueNum(workerCount + miscThread + 1)  //[0...mQueueNum)
        , mPendingNodes(new PendingNodeQueue[mQueueNum]), mIsRunning(true) {
    for (int i = 0; i < mQueueNum; i++) {
        mPendingNodes[i].Init(capacity);
    }

    mPendingTask.reserve(MAX_PENDING_NODES);
}

bool Dispatcher::Enqueue(int32_t fromID, DEventNode *pNode) {
    if (LIKELY(mIsRunning)) {
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

void Dispatcher::VisitNode(DEventNode *pNode, int32_t level, int32_t workflowId) {
    //一个节点有多个前驱节点时，只能通过一个前驱节点被Dispatch。
    if (!pNode->HasScheduled(workflowId)) {
        pNode->SetDispatchedID(workflowId);
        //1. select worker
        //2. create task
        //3. acquire locks (Before Worker::Enqueue)
        //4. visit children

        auto &successors = pNode->GetSuccessors();
        for (auto pSuccessor : successors) {
            VisitNode(pSuccessor, level + 1, workflowId);
        }

        auto idxWorker = SelectWorker(pNode);
        auto pTargetWorker = Context::GetWorker(idxWorker);
        Task *pTask = pTargetWorker->AllocateTaskFromPool(workflowId, pNode);
        mPendingTask.push_back(pTask);
    }
    //LOG_ERROR("Node-%s has been scheduled!!!!\n", pNode->GetName().c_str());
};


#include <stdio.h>

void Dispatcher::Run(void) {
    int32_t workflowId = 0;
#ifdef RUN_UNTIL_NOMORE_TASK
    bool bye = true;
    while (LIKELY(mIsRunning || !bye)) {
//        LOG_INFO(mLog, "Dispatcher--:XX" << bye);
        bye = true;
#else
    while(mIsRunning){
#endif
        static int DispatchIndex = ThreadIndex[ThreadType::DISPATCHER].first;
        //Does dispatcher dispatch the task to itself???
        int32_t nWorkflowDelta = 0;
        mPendingTask.clear();
        for (WorkerId fromWorker = DispatchIndex; fromWorker < mQueueNum; ++fromWorker) {
            auto &pNodeQueue = mPendingNodes[fromWorker];
            // printf("Dispatcher consumes Queue-%d\n", fromWorker);
#ifdef RUN_UNTIL_NOMORE_TASK
            pNodeQueue.consume_all([this,&bye,&nWorkflowDelta, workflowId](DEventNode* pNode) {
                bye = false;
#else
            pNodeQueue.consume_all([this,&nWorkflowDelta, workflowId](DEventNode* pNode) {
#endif
                nWorkflowDelta = 1;
                VisitNode(pNode, 0, workflowId);
            });
        }

        for(auto itTask = mPendingTask.rbegin(); itTask != mPendingTask.rend(); ++itTask)
        {
            auto pTask = *itTask;
            auto pNode = pTask->GetNode();
            auto &precessor = pNode->GetPrecursors();
            for (auto pPrecessor : precessor) {
                pPrecessor->GetMutex().LockShared(pTask);
            }
            pNode->GetMutex().Lock(pTask);
            pTask->Enqueue(DispatchIndex, nullptr);
            LOG_INFO(mLog, "Dispatcher push node:" << pNode->GetName() << " to Worker-" << pTask->GetWorkerId()
                                                   << " @workflow:" << workflowId << " task:" << pTask);
        }

        workflowId += nWorkflowDelta;
//        LOG_INFO(mLog, "Dispatcher--:YY workflow:" << workflowId);
    }
    LOG_DEBUG(mLog, "Dispatcher jump out main loop");
    mStopFinished = true;
}

void Dispatcher::Stop(void)
{
    mIsRunning = false;
}

void Dispatcher::Join( void )
{
    while(UNLIKELY(!mStopFinished))
    {
        LOG_DEBUG(mLog, "Dispatcher stopping:" << mStopFinished);
    }
}
