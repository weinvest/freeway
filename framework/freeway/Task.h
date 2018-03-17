//
// Created by shgli on 17-9-27.
//

#pragma once

#include <atomic>
#include <boost/context/all.hpp>
#include <framework/freeway/ITask.h>
#include <unordered_map>
#include <vector>


namespace ctx = boost::context;
class DEventNode;
class DummyNode;


class Task: public ITask
{
public:
    Task();

    Task(Task&& other) = delete;
    Task(const Task&) = delete;
    Task& operator =(const Task&) = delete;

    const std::string& GetName( void ) override ;

    //void Acquire( void ) override;
    //void Release( void ) override;

    void Resume( void ) override;

//    bool IsScheduleAble( void ) const;

    WorkflowID_t GetWorkflowID( void ) const { return mWorkflowId; }
    void Suspend(void) override;
private:
    void RunNode( void );

    ctx::continuation mMainContext;
    ctx::continuation mTaskContext;
    bool mLockAcquired;
 //   std::atomic_int mWaitingLockCount;
 //   bool mAccepted;
};



