//
// Created by shgli on 17-10-12.
//

#ifndef ARAGOPROJECT_SHAREDMUTEX_H
#define ARAGOPROJECT_SHAREDMUTEX_H

#include <boost/circular_buffer.hpp>
#include <common/DSpscQueue.hpp>
#include "DEventNode.h"

class DEventNode;
class Task;
class SharedMutex
{
    struct alignas(8) WaiterType
    {
        WaiterType(Task* ptr, bool write):pTask(ptr), IsWriter(write) {}
        WaiterType( void ): pTask(nullptr), IsWriter(false) {}
        Task* pTask;
        bool IsWriter;

        inline bool IsNull( void ) const { return nullptr == pTask; }
        inline void Reset( void ) { pTask = nullptr; }
        static inline WaiterType Null( void ) { return WaiterType(); }

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
    void LockShared(Task* pTask);

    void WaitSharedLock4(Task* pTask);

    bool TrySharedLock4(Task* pTask);
//后继节点结束后,释放掉本身对应的mReaders(无论是否调用过LockShared)计数
//除非Task不存在，自然不会Unlock，后继节点也不应该UnlockShared
    void UnlockShared(Task* pTask);

//每个节点DoProcess前都调用Lock
    void Lock(Task* task);

    void WaitLock4(Task* pTask);

    bool TryLock4(Task* pTask);

    //当前节点DoProcess结束就可以调用Unlock(即使Unlock后，Task[Nextflow] 依然会被mReaders阻止)
    void Unlock(Task* task);

private:
    //如果后继节点没有调用前驱的GetValue, 则不用唤醒 NextTask?
    bool Wake();

    DEventNode* mOwner{nullptr};

    std::atomic<int> mReaders{0};
    std::atomic_flag mIsInWaking ATOMIC_FLAG_INIT;
    WaiterBufferType mWaiters;
    WaiterBufferType::Holder* mLastLockObject{nullptr};
    WaitingWriterType mWaitingWriterWorkflowIds{1024};
};


#endif //ARAGOPROJECT_SHAREDMUTEX_H
