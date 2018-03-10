//
// Created by shgli on 17-9-27.
//

#include "DEventNode.h"
#include "types.h"
#include <exception>
#include <boost/exception/all.hpp>
#include "SharedMutex.h"

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
bool DEventNode::HasSharedLock4(ITask* pTask) const
{
    return mMutex->HasSharedLock4(pTask);
}

int32_t DEventNode::Process(WorkflowID_t workflowId) noexcept
{
    try
    {
        return DoProcess(workflowId);
    }
    catch(const boost::exception& ex)
    {
       LOG_ERROR(mLog, GetName() << " exception at DoProcess:" << boost::diagnostic_information(ex));
    }
    catch(const std::exception& ex)
    {
        LOG_ERROR(mLog, GetName() << " exception at DoProcess:" << ex.what());
    }
    catch(...)
    {
        LOG_ERROR(mLog, GetName() << " exception at DoProcess.");
    }

    return NoRaiseSuccessor;
}
/*too late!!!*/
bool DEventNode::HasScheduled(WorkflowID_t flow)
{
    return flow <= mLastDispatchedflowId;
}

