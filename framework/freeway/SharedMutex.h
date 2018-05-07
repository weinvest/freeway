//
// Created by shgli on 17-10-12.
//

#ifndef ARAGOPROJECT_SHAREDMUTEX_H
#define ARAGOPROJECT_SHAREDMUTEX_H
#include <atomic>
#include "common/DSpscArray.h"
#include "framework/freeway/types.h"
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

        bool operator==(const WaiterType& o) const { return pTask == o.pTask && IsWriter == o.IsWriter; }
        bool operator!=(const WaiterType& o) const { return !(*this == o); }
    };

public:

    using WaiterBufferType = DSpscArray<WaiterType ,int32_t, std::atomic_int>;
    using WaitingWriterType = DSpscArray<WorkflowID_t>;

    SharedMutex(DEventNode* pOwner);
    SharedMutex(const SharedMutex&) = delete;
    SharedMutex& operator=(const SharedMutex&) = delete;

    void LockShared(Task* pTask);

    void WaitSharedLock4(Task* pTask);

    bool TrySharedLock4(Task* pTask);
    void UnlockShared(Task* pTask);

    void Lock(Task* task);

    void WaitLock4(Task* pTask);

    bool TryLock4(Task* pTask);

    void Unlock(Task* task);

    WaiterBufferType& GetWaiters() { return mWaiters; }
    WorkflowID_t GetFirstWaittingWriter();

    int32_t GetReaderCount( void ) { return mReaders.load(std::memory_order_relaxed); }
private:
    void Wake(Task* pTask);

    DEventNode* mOwner{nullptr};

    alignas(64) std::atomic<int> mReaders{0};
    WaiterBufferType mWaiters;
    WaitingWriterType mWaitingWriterWorkflowIds;
};


#endif //ARAGOPROJECT_SHAREDMUTEX_H
