//
// Created by shgli on 17-9-27.
//

#include "DEventNode.h"
#include "types.h"
#include <exception>
#include <boost/exception/all.hpp>

#include <iostream>

int32_t DEventNode::Process(WorkflowID_t workflowId) noexcept
{
    try
    {
        auto ret = DoProcess(workflowId);
        mLastWorkflowId = workflowId;
        return ret;
    }
    catch(const boost::exception& ex)
    {
        std::cerr<<GetName()<<" LINE-"<<__LINE__<< " exception at DoProcess:" << boost::diagnostic_information(ex)<<std::endl;
    }
    catch(const std::exception& ex)
    {
        std::cerr<<GetName()<<" LINE-"<<__LINE__ << " exception at DoProcess:" << ex.what()<<std::endl;
    }
    catch(...)
    {
        std::cerr<<GetName()<<" LINE-"<<__LINE__ << " exception at DoProcess."<<std::endl;
    }
    return NoRaiseSuccessor;
}
/*too late!!!*/
bool DEventNode::HasScheduled(WorkflowID_t flow)
{
    return flow <= mLastDispatchedflowId;
}

