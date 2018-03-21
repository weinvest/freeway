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
DEventNode::DEventNode()
:mMutex(new SharedMutex(this)){

}

void DEventNode::Connect(DEventNode* pSuccessor)
{
    AddSuccessor(pSuccessor);
    pSuccessor->AddPrecursor(this);
}

SharedMutex& DEventNode::GetMutex()
{
    return *mMutex;
}

int32_t DEventNode::Process(ITask* pTask, WorkflowID_t workflowId) noexcept
{
    int32_t result = NoRaiseSuccessor;
    if(mIsAcceptTrigger) {
        mMutex->WaitLock4(pTask);
        try {
            result = DoProcess(workflowId);
        }
        catch (const boost::exception &ex) {
            LOG_ERROR(mLog, GetName() << " exception at DoProcess:" << boost::diagnostic_information(ex));
        }
        catch (const std::exception &ex) {
            LOG_ERROR(mLog, GetName() << " exception at DoProcess:" << ex.what());
        }
        catch (...) {
            LOG_ERROR(mLog, GetName() << " exception at DoProcess.");
        }
    }

    for (auto precursor : GetPrecursors()) {
        LOG_INFO(mLog, "Coroutine Task for %s[%d] RunNode is over, Unlockshared(%s) on Worker-%d", GetName(),
                 mWorkflowId, precursor->GetName(), mWorkerId);
        precursor->GetMutex().UnlockShared(pTask);
    }

    for(auto successor : GetSuccessors()){
        successor->Raise(this, result);
    }

    LOG_INFO(mLog, "Coroutine Task for %s[%d] RunNode is over, Unlocking...", GetName(), mWorkflowId);
    mMutex->Unlock(pTask);
    return result;
}

void DEventNode::Raise(DEventNode* pPrecessor, int32_t reason)
{
    try
    {
        if((this == pPrecessor) || (NoRaiseSuccessor != reason && OnRaised(pPrecessor, reason)))
        {
            mIsAcceptTrigger = true;
        }
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
    Context::GetDispatcher()->Enqueue(fromThread, this);
}

/*too late!!!*/
bool DEventNode::HasScheduled(WorkflowID_t flow)
{
    return flow <= mLastDispatchedflowId;
}

