//
// Created by shgli on 17-9-27.
//

#ifndef ARAGOPROJECT_DISPATCHER_H
#define ARAGOPROJECT_DISPATCHER_H

#include <array>
#include "common/DSpscQueue.hpp"
#include "ITask.h"
class DEventNode;
class Dispatcher
{
public:
    Dispatcher(int32_t workerCount, int32_t miscThreads);
    ~Dispatcher();

    bool Enqueue(int32_t fromID, DEventNode* );

    void Run( void );
    void Stop() {mIsRunning = false;}
private:
    int32_t SelectWorker(DEventNode* );
    ITask* VisitNode (DEventNode* pNode, int32_t level);

    static constexpr int32_t MAX_PENDING_NODES = 128;
    const int32_t mWorkerCount;
    const int32_t mMiscThreadCount;
    const int32_t mQueueNum;

    using PendingNodeQueue = DSpscQueue<DEventNode*>;
    PendingNodeQueue* mPendingNodes;

    std::vector<ITask*> mPendingTask;
    bool mIsRunning;
};


#endif //ARAGOPROJECT_DISPATCHER_H
