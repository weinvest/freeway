//
// Created by shgli on 17-10-13.
//

#ifndef ARAGOPROJECT_LOCKPTR_H
#define ARAGOPROJECT_LOCKPTR_H
#include <type_traits>
#include <framework/freeway/types.h>
#include <cassert>
#include <string>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <array>
#include <utility>
#include <unordered_map>
#include <framework/freeway/ITask.h>
#include <framework/freeway/Worker.h>
#include <framework/freeway/Context.h>
#include <mutex>

//
//template <typename T>
//class RemotePtr
//{
//public:
////RemotePtr 与 DEventNode 循环依赖，
////先构造RemotePtr，才能将pLock加入到DEventNode的依赖关系中
//    RemotePtr(T* remote): mRemote(remote) {}
//    RemotePtr(RemotePtr&& other)
//    {
//        mRemote  = std::exchange(other.mRemote, nullptr);
//    }
//
//    RemotePtr(const RemotePtr&) = delete;
//    RemotePtr& operator=(const RemotePtr&) = delete;
//
//    T* operator-> ()
//    {
//        if(!mRemote->LockShared())
//        {
//        #if 0
//            if(GetCurrentTask()->GetReadSequence() == 2) //Unlock时，nothing to do
//            {
//                LOG_WARN("RemotePtr: ReadTask[%p]-%s[%d] give up suspending", GetCurrentTask(), GetCurrentTask()->GetName().c_str(),
//                                GetCurrentTask()->GetWorkflowId());
//                return mRemote;
//            }
//            else
//            {
//                /*
//                1:LockShared 失败，Wake()中正在等待ReaderSuspend
//                3:Reader有多个前驱，只有一个前驱节点能将其ReadSequence设置为3，成功者负责唤醒Reader
//                */
//                assert(GetCurrentTask()->GetReadSequence() == 1 || GetCurrentTask()->GetReadSequence() == 3);
//                LOG_WARN("RemotePtr: ReadTask[%p]-%s[%d] try suspending", GetCurrentTask(), GetCurrentTask()->GetName().c_str(),
//                                GetCurrentTask()->GetWorkflowId());
//                GetCurrentTask()->Suspend(TaskStatus::SUSPEND_BY_READ);
//            }
//        #else
//           LOG_WARN("RemotePtr: ReadTask[%p]-%s[%d] is suspending", GetCurrentTask(), GetCurrentTask()->GetName().c_str(),
//                                GetCurrentTask()->GetWorkflowId());
//
//        #endif
//        }
//        return mRemote;
//    }
//
//    T* get()
//    {
//        return mRemote;
//    }
//
//public:
///*
//    ITask* operator()(WorkflowID_t flowID)
//    {
//        auto search = mLinkedTasksMap.find(flowID);
//        if(search != mLinkedTasksMap.end())
//            return search->second;
//        //后继节点先于当前节点运行完毕被回收，会导致找不到
//        LOG_ERROR("RemotePtr[%p] can't find Task for %s flow=%d (May be it's destroyed)", this, mRemote->GetName().c_str(), flowID);
//        return nullptr;
//    }
//*/
//private:
//    T* mRemote;
////    T* mCurrent;
//};
//
//
//
//
//template <typename T>
//RemotePtr<T> make_locked(T* impl)
//{
//    RemotePtr<T> l(impl);
//    return l ;
//}
#endif //ARAGOPROJECT_LOCKPTR_H
