//
// Created by shgli on 17-10-9.
//

#ifndef ARAGOPROJECT_ITASK_H_H
#define ARAGOPROJECT_ITASK_H_H

#include <string>
#include "types.h"
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

class DEventNode;
template <typename T>
class LockPtr;
class ITask
{
public:

    ITask(WorkflowID_t workflowId, int32_t workerId):
            mWorkflowId(workflowId)
            ,mWorkerId(workerId)
    {};

    ITask() {}
    int32_t GetWorkflowId() const { return mWorkflowId; }
    int32_t GetWorkerId() {return mWorkerId;}

    void Update(WorkflowID_t flow, WorkerID_t worker, DEventNode* pNode)
    {   
        mWorkflowId = flow;
        mWorkerId=worker;
        mNodePtr = pNode;
    }   


    virtual int32_t GetWaitingLockCount( void ) const { return 0; }
    virtual ~ITask() {}

    virtual void SetLock() = 0;
    virtual void SetShared(ITask* pTask) = 0;

 //   virtual bool IsScheduleAble( void ) const { return true; };
    virtual const std::string& GetName( void ) const = 0;
//    virtual void Acquire( void ) = 0;
//    virtual void Release( void ) = 0;
    virtual void Resume( void ) = 0;
    virtual void Suspend( TaskStatus reason ) = 0;
    virtual void PrepareContext() = 0;

    bool IsInit(void) {std::atomic_thread_fence(std::memory_order_acquire); return mStatus == TaskStatus::INITIALIZING; }
    bool IsDispatched(void) {std::atomic_thread_fence(std::memory_order_acquire); return mStatus == TaskStatus::DISPATCHED; }
    bool IsReady(void) const {std::atomic_thread_fence(std::memory_order_acquire); return mStatus == TaskStatus::READY; }
    bool IsSuspendRead(void) {std::atomic_thread_fence(std::memory_order_acquire); return mStatus == TaskStatus::SUSPEND_BY_READ; }
    bool IsSuspendWrite(void) {std::atomic_thread_fence(std::memory_order_acquire); return mStatus == TaskStatus::SUSPEND_BY_WRITE; }
    bool IsStop(void) {std::atomic_thread_fence(std::memory_order_acquire); return mStatus == TaskStatus::STOPPED; }
   
    void SetInit() { mStatus = TaskStatus::INITIALIZING; std::atomic_thread_fence(std::memory_order_release);}
    void SetDispatched() { mStatus = TaskStatus::DISPATCHED; std::atomic_thread_fence(std::memory_order_release);}
    void SetReady() { mStatus = TaskStatus::READY; std::atomic_thread_fence(std::memory_order_release);} 
    bool SetSuspend(TaskStatus reason)
    {
        if(IsTryResumed())
        {
            LOG_ERROR("%s[%p]: TryResumed is set!!!!!!!!", GetName().c_str(), this);
            return false;
        }
        mStatus = reason; std::atomic_thread_fence(std::memory_order_release);
        return true;
    }
    void SetStop() { mStatus = TaskStatus::STOPPED; std::atomic_thread_fence(std::memory_order_release);}

    void TryToResume() {mTryResumed.store(true, std::memory_order_release);}
    //it has to be called before suspend to avoid of critical conditon when resuming in precursor
    bool IsTryResumed() {return mTryResumed.load(std::memory_order_acquire);}
protected:
    std::atomic<bool> mTryResumed{false};
    WorkflowID_t mWorkflowId;
    WorkerID_t mWorkerId;
    DEventNode* mNodePtr {nullptr};
    TaskStatus mStatus{TaskStatus::INITIALIZING};
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
    IdleTask() :ITask(-1, -1){SetReady();};

    const std::string& GetName( void ) const { return mName;};

    virtual void SetLock() {}
    virtual void SetShared(ITask* pTask) {}

    void Acquire( void ) {}
    void Release( void ) {}
    void Suspend( TaskStatus) {}
    void Resume( void )  {}
    void PrepareContext() {}
private:
    std::string mName{"IdleTask"};
};

#endif //ARAGOPROJECT_ITASK_H_H
