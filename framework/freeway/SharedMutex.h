//
// Created by shgli on 17-10-12.
//

#ifndef ARAGOPROJECT_SHAREDMUTEX_H
#define ARAGOPROJECT_SHAREDMUTEX_H

#include <boost/circular_buffer.hpp>
#include <common/DSpscQueue.hpp>
#include "DEventNode.h"

class DEventNode;
class ITask;
class SharedMutex
{
    struct WaiterType
    {
        WaiterType(ITask* ptr, bool write):pTask(ptr), IsWriter(write) {}
        WaiterType( void ): pTask(nullptr), IsWriter(false) {}
        ITask* pTask;
        bool IsWriter;

        bool operator==(const WaiterType& o) const { return pTask == o.pTask && IsWriter == o.IsWriter; }
        bool operator!=(const WaiterType& o) const { return !(*this == o); }
    };

public:
//    using WaiterBufferType = boost::circular_buffer<WaiterType>;
//    using WaitingWriterType = boost::circular_buffer<WorkflowID_t >;

    using WaiterBufferType = DSpscQueue<WaiterType>;
    using WaitingWriterType = boost::circular_buffer<WorkflowID_t >;

    SharedMutex(DEventNode* pOwner);
    SharedMutex(const SharedMutex&) = delete;
    SharedMutex& operator=(const SharedMutex&) = delete;

    /*
        mReaders 代表了当前有多少个Reader Task处于Runing状态
        mWriter 代表当前flow是否有Writer存在，
        1. 为了保证线程安全，Dispatcher中push_back; 当前节点运行结束，将所有的ReadWaiter + WriteWaiter in NextFlow 都pop_back
        2. Resume ReadWaiter时，mReaders++, 用来阻止下一轮的Writer运行;所以所有的 Reader运行结束时都要UnlockShared, mReaders--
    */


//后继节点调用(有可能比前驱节点先运行, 或者前驱节点运行结束之后才运行)
    bool LockShared(ITask* pTask);

    bool HasSharedLock4(ITask* pTask);
//后继节点结束后,释放掉本身对应的mReaders(无论是否调用过LockShared)计数
//除非Task不存在，自然不会Unlock，后继节点也不应该UnlockShared
    void UnlockShared(ITask* pTask);

//每个节点DoProcess前都调用Lock
    bool Lock(ITask* task);

    bool HasLock4(ITask* pTask);

    //当前节点DoProcess结束就可以调用Unlock(即使Unlock后，Task[Nextflow] 依然会被mReaders阻止)
    void Unlock(ITask* task);

private:
    //如果后继节点没有调用前驱的GetValue, 则不用唤醒 NextTask?
    bool Wake();

    DEventNode* mOwner{nullptr};
    ITask* mWriter{nullptr};
    std::atomic<int> mReaders{0};
    WaiterBufferType mWaiters;
    WaiterBufferType::Holder* mLastLockObject{nullptr};
    WaitingWriterType mWaitingWriterWorkflowIds{1024};
};


#endif //ARAGOPROJECT_SHAREDMUTEX_H
