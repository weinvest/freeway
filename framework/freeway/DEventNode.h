//
// Created by shgli on 17-9-27.
//

#ifndef ARAGOPROJECT_DEVENTNODE_H
#define ARAGOPROJECT_DEVENTNODE_H
#include "common/Types.h"
const int32_t NoRaiseSuccessor = -1;

using WorkflowID_t = uint64_t;
class ITask;
class SharedMutex;
class DEventNode
{
public:
    DEventNode();

//    void ShowMutexInfo() {printf("%s's ShareMutex[%p]\n", mName.c_str(), &mMutex);}

    const dvector<DEventNode*>& GetSuccessors( void ) const { return mSuccessors; }
    const dvector<DEventNode*>& GetPrecursors( void ) const { return mPrecursors; }

    void AddPrecursor(DEventNode* pNode) {mPrecursors.push_back(pNode);}
    void AddSuccessor(DEventNode* pNode) {mSuccessors.push_back(pNode);}

    void Connect(DEventNode* pSuccessor);

    SharedMutex& GetMutex();

    const std::string& GetName() {return mName;}
    void SetName(const std::string& name) {mName = name;}

    bool HasScheduled(WorkflowID_t workflowId);
    int32_t Process(ITask* pTask, WorkflowID_t workflowId) noexcept ;

    void SetDispatchedID(WorkflowID_t workflowId) {mLastDispatchedflowId = workflowId;}

    WorkflowID_t GetDispatchedID() const {return mLastDispatchedflowId;}

    void RaiseSelf( void );
    void RaiseSelf(int32_t fromThread);
protected:
    dvector<DEventNode*> mSuccessors;
    dvector<DEventNode*> mPrecursors;
    void Raise(DEventNode* precursor, int32_t reason);
private:
    virtual int32_t DoProcess(WorkflowID_t workflowId) = 0;
    virtual bool OnRaised(DEventNode* precursor, int32_t reason);

    WorkflowID_t mLastDispatchedflowId{0};
    SharedMutex* mMutex;
    std::string mName;
    bool mIsAcceptTrigger{false};
};

class DummyNode : public DEventNode
{
public:
    int32_t DoProcess(WorkflowID_t workflowId)
    {
//        std::cout<<"DummyNode is used to init spsc_queue\n";
        return 0;
    }
};
#endif //ARAGOPROJECT_DEVENTNODE_H
