//
// Created by shgli on 17-9-27.
//

#ifndef ARAGOPROJECT_DISPATCHER_H
#define ARAGOPROJECT_DISPATCHER_H

#include <array>
#include "common/DSpscArray.h"
#include "utils/DLog.h"
class DEventNode;
class Task;
class Dispatcher
{
public:
    Dispatcher(int32_t workerCount, int32_t miscThreads);
    ~Dispatcher();
    bool IsRunning( void ) const { return !mStopFinished.load(std::memory_order_relaxed); }
    bool Enqueue(int32_t fromID, DEventNode* );

    void Run( void );
    void Join( void );
    void Stop();
    void WaitStart();
private:
    int32_t SelectWorker(DEventNode* );
    void VisitNode (DEventNode* pNode, int32_t level, int32_t workflowId);

    static constexpr int32_t MAX_PENDING_NODES = 128;
    const int32_t mWorkerCount;
    const int32_t mMiscThreadCount;
    const int32_t mQueueNum;

    using PendingNodeQueue = DSpscArray<DEventNode*>;
    PendingNodeQueue* mPendingNodes;

    std::vector<Task*> mPendingTask;
    bool mIsRunning{false};
    std::atomic<bool> mStopFinished{false};
    Logger mLog;
};


#endif //ARAGOPROJECT_DISPATCHER_H
