//
// Created by shgli on 17-9-27.
//

#ifndef ARAGOPROJECT_DEVENTNODE_H
#define ARAGOPROJECT_DEVENTNODE_H

#include "common/Types.h"
#include "framework/freeway/LockPtr.h"
#include "framework/freeway/TaskList.h"
const int32_t NoRaiseSuccessor = -1;

using WorkflowID_t = int32_t;
class Task;
class TaskList;
class SharedMutex;
class DEventNodeSpecial;
class LockPtrBase;
class DEventNode
{
public:
    DEventNode();

    const dvector<DEventNode*>& GetSuccessors( void ) const { return mSuccessors; }
    const dvector<DEventNode*>& GetPrecursors( void ) const { return mPrecursors; }

    SharedMutex& GetMutex();

    void SetAlwaysAccept(bool accept) { mAlwaysAccept = accept; }
    bool IsAlwaysAccept( void ) const { return mAlwaysAccept; }

    const std::string& GetName() {return mName;}
    void SetName(const std::string& name) {mName = name;}

    LockPtrBase Connect(DEventNode* pSuccessor);

    int32_t Process(Task* pTask, WorkflowID_t workflowId) noexcept ;

    void SetDispatchedTask(Task* pTask) { mLastDispatchedTask = pTask;}
    Task* GetDispatchedTask( void ) { return mLastDispatchedTask;}
    bool IsDispatched(WorkflowID_t workflowId);

    WorkflowID_t GetLastWorkflowId() const { return mLastWorkflowId; }

    void RaiseSelf( void );
    void RaiseSelf(int32_t fromThread);

#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    auto& GetWaittingList(int32_t workerId) { return mWaitingTasks[workerId]; }
    auto& GetUnschedulableList(int32_t workerId) { return mUnschedulableTasks[workerId]; }
#endif

protected:
    void AddPrecursor(DEventNode* pNode) {mPrecursors.push_back(pNode);}
    void AddSuccessor(DEventNode* pNode) {mSuccessors.push_back(pNode);}

    dvector<DEventNode*> mSuccessors;
    dvector<DEventNode*> mPrecursors;

    bool Raise(DEventNode* precursor, int32_t reason);
private:
    virtual int32_t DoProcess(WorkflowID_t workflowId) = 0;
    virtual bool OnRaised(DEventNode* precursor, int32_t reason);
    DEventNodeSpecial* EnsureSpecial(DEventNode* pPrecessor);

    friend class LockPtrBase;
    Task* mLastDispatchedTask{nullptr};
    WorkflowID_t mLastWorkflowId{0};
    SharedMutex* mMutex;
    std::string mName;
    bool mAlwaysAccept{false};
    std::unordered_map<DEventNode*, DEventNodeSpecial*> mPrecursorSpecials;

#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    friend class Task;
    typedef TaskList TaskQueue;
    TaskQueue* mWaitingTasks{nullptr};
    TaskQueue* mUnschedulableTasks{nullptr};
#endif
};

#endif //ARAGOPROJECT_DEVENTNODE_H
