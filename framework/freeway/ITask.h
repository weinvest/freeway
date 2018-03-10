//
// Created by shgli on 17-10-9.
//

#ifndef ARAGOPROJECT_ITASK_H_H
#define ARAGOPROJECT_ITASK_H_H

#include <string>
#include <framework/freeway/types.h>

enum TaskStatus
{
    INITIALIZING = 1,
    DISPATCHED,
    READY,
    SUSPEND_BY_READ,
    SUSPEND_BY_WRITE,
    RUNNING,
    STOPPED
};

inline unsigned long CompositeStopper(unsigned long stopper, unsigned long sequence)
{
    return (stopper|(sequence<<48));
}
//#define ExtractStopper(composition) reinterpret_cast<SharedMutex*>(composition&(1l<<47-1))
//#define ExtractSequence(composition) (composition>>48l)
class DEventNode;
class Worker;
template <typename T>
class LockPtr;
class ITask
{
public:

    ITask() = default;

    int32_t GetWorkflowId() const { return mWorkflowId; }
    int32_t GetWorkerId() const;
    Worker* GetWorker() const { return mWorker; }
    DEventNode* GetNode() { return mNodePtr; }

    void Update(WorkflowID_t flow, Worker* worker, DEventNode* pNode);

    virtual int32_t GetWaitingLockCount( void ) const { return mWaitingLockCount; }
    virtual ~ITask() {}

    virtual const std::string& GetName( void ) = 0;

    virtual void Resume( void ) = 0;
    virtual void Suspend( TaskStatus reason ) = 0;

    void* GetWaited() { return mWaited; }

    void SetWaited(void* pWaited) { mWaited = pWaited; }

    void DecreaseWaitingLockCount( void ) { --mWaitingLockCount; }
protected:

    Worker* mWorker{nullptr};
    DEventNode* mNodePtr {nullptr};

    void* mWaited{nullptr};

    WorkflowID_t mWorkflowId{0};
    int32_t mWaitingLockCount;
};

struct TaskCompare
{
    bool operator() (ITask* pTask1, ITask* pTask2)
    {
        return pTask1->GetWorkflowId() < pTask2->GetWorkflowId()
               || pTask1->GetWaitingLockCount() < pTask2->GetWaitingLockCount();
    }
};



class IdleTask: public ITask
{
public:
    IdleTask() {};

    const std::string& GetName( void ) { return mName;};

    virtual void SetLock() {}
    virtual void SetShared(ITask* pTask) {}

    void Acquire( void ) {}
    void Release( void ) {}
    void Suspend( TaskStatus) {}
    void Resume( void )  {}
private:
    std::string mName{"IdleTask"};
};

#endif //ARAGOPROJECT_ITASK_H_H
