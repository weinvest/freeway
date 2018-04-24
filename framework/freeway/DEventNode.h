//
// Created by shgli on 17-9-27.
//

#ifndef ARAGOPROJECT_DEVENTNODE_H
#define ARAGOPROJECT_DEVENTNODE_H

#include "common/Types.h"
#include "framework/freeway/LockPtr.h"
const int32_t NoRaiseSuccessor = -1;

using WorkflowID_t = uint64_t;
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

    const std::string& GetName() {return mName;}
    void SetName(const std::string& name) {mName = name;}

    LockPtrBase Connect(DEventNode* pSuccessor);

    int32_t Process(Task* pTask, WorkflowID_t workflowId) noexcept ;

    void SetDispatchedID(WorkflowID_t workflowId) {mLastDispatchedflowId = workflowId;}
    bool HasScheduled(WorkflowID_t workflowId);

    WorkflowID_t GetLastWorkflowId() const { return mLastWorkflowId; }

    void RaiseSelf( void );
    void RaiseSelf(int32_t fromThread);

#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    auto& GetWaittingList(int32_t workerId) { return mWaitingTasks[workerId]; }
#endif

protected:
    void AddPrecursor(DEventNode* pNode) {mPrecursors.push_back(pNode);}
    void AddSuccessor(DEventNode* pNode) {mSuccessors.push_back(pNode);}

    dvector<DEventNode*> mSuccessors;
    dvector<DEventNode*> mPrecursors;
    void Raise(DEventNode* precursor, int32_t reason);
private:
    virtual int32_t DoProcess(WorkflowID_t workflowId) = 0;
    virtual bool OnRaised(DEventNode* precursor, int32_t reason);
    DEventNodeSpecial* EnsureSpecial(DEventNode* pPrecessor);

    friend class LockPtrBase;
    WorkflowID_t mLastDispatchedflowId{0};
    WorkflowID_t mLastWorkflowId{0};
    SharedMutex* mMutex;
    std::string mName;
    bool mIsAcceptTrigger{false};
    std::unordered_map<DEventNode*, DEventNodeSpecial*> mPrecursorSpecials;

#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
    friend class Task;
    typedef std::list<Task*> TaskQueue;
    TaskQueue* mWaitingTasks{nullptr};
#endif
};

#endif //ARAGOPROJECT_DEVENTNODE_H
