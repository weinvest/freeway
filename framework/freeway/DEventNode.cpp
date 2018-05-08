//
// Created by shgli on 17-9-27.
//

#include "DEventNode.h"
#include "types.h"
#include <exception>
#include <boost/exception/all.hpp>
#include "SharedMutex.h"
#include "Context.h"
#include "Dispatcher.h"
#include "DEventNodeSpecial.h"
#include "TaskList.h"
#include "Task.h"
#define mLog Context::GetLog()
DEventNode::DEventNode()
     :mMutex(new SharedMutex(this))
#ifdef _USING_MULTI_LEVEL_WAITTING_LIST
     ,mWaitingTasks(new TaskQueue[8])
     ,mUnschedulableTasks(new TaskQueue[8])
#endif
{
}

LockPtrBase DEventNode::Connect(DEventNode* pSuccessor)
{
    auto itSpecial = pSuccessor->mPrecursorSpecials.find(this);
    if(itSpecial != pSuccessor->mPrecursorSpecials.end())
    {
        return LockPtrBase(this, (itSpecial->second));
    }

    auto pSpecial = new DEventNodeSpecial();
    pSuccessor->AddPrecursor(this);
    AddSuccessor(pSuccessor);
    pSuccessor->mPrecursorSpecials[this] = pSpecial;
    return LockPtrBase(this, pSpecial);
}

SharedMutex& DEventNode::GetMutex()
{
    return *mMutex;
}

int32_t DEventNode::Process(Task* pTask, WorkflowID_t workflowId) noexcept
{
    int32_t result = NoRaiseSuccessor;
    try
    {
        result = DoProcess(workflowId);
    }
    catch (const boost::exception &ex)
    {
        LOG_ERROR(mLog, GetName() << " exception at DoProcess:" << boost::diagnostic_information(ex));
    }
    catch (const std::exception &ex)
    {
        LOG_ERROR(mLog, GetName() << " exception at DoProcess:" << ex.what());
    }
    catch (...)
    {
        LOG_ERROR(mLog, GetName() << " exception at DoProcess.");
    }

    mLastWorkflowId = workflowId;

    return result;
}

bool DEventNode::Raise(DEventNode* pPrecessor, int32_t reason)
{
    try
    {
        return OnRaised(pPrecessor, reason);
    }
    catch(const boost::exception& ex)
    {
        LOG_ERROR(mLog, GetName() << " exception at OnRaised:" << boost::diagnostic_information(ex));
    }
    catch(const std::exception& ex)
    {
        LOG_ERROR(mLog, GetName() << " exception at OnRaised:" << ex.what());
    }
    catch(...)
    {
        LOG_ERROR(mLog, GetName() << " exception at OnRaised.");
    }

    return false;
}

bool DEventNode::OnRaised(DEventNode* precursor, int32_t reason)
{
    return true;
}

void DEventNode::RaiseSelf( void )
{
    RaiseSelf(Context::GetThreadId());
}

void DEventNode::RaiseSelf(int32_t fromThread)
{
    assert(-1 != fromThread);
    Context::GetDispatcher()->Enqueue(fromThread, this);
}

bool DEventNode::IsDispatched(WorkflowID_t flow)
{
    return nullptr != mLastDispatchedTask && flow <= mLastDispatchedTask->GetWorkflowId();
}

