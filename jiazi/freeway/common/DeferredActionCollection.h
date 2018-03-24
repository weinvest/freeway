#ifndef DEFERREDACTIONCOLLECTION_H
#define DEFERREDACTIONCOLLECTION_H
#include <deque>
#include <atomic>
#include <functional>
#include "common/DSpinLock.h"
#include "common/DTimer.h"

struct RunStatus
{
    enum
    {
        Stoped = 0,
        Started = 1,
        Suspended = 2
    };
};

struct DeferredMode
{
    enum type
    {
        FILO = 0,
        FIFO = 1
    };
};

template<typename T>
class DeferredActionCollection
{
public:
    typedef T* ValuePtr;
    typedef std::function<void (ValuePtr)> Action;

    DeferredActionCollection(Action act, DeferredMode::type mode = DeferredMode::FILO)
        :mAction(act)
        ,mStatus(RunStatus::Stoped)
        ,mHasTaskInRuning(false)
        ,mEnabled(true)
        ,mMode(mode)
    {
    }

    int Count( void ) const
    {
        SPINLOCK(mLock)
        {
            return mQueue.size();
        }

        return 0;
    }

    void Start()
    {
        SPINLOCK (mLock)
        {
            if (mStatus == RunStatus::Stoped)
            {
                mStatus = RunStatus::Started;
                EmitOne();
            }
        }
    }

    void Stop()
    {
        SPINLOCK (mLock)
        {
            if (mStatus != RunStatus::Stoped)
            {
                mStatus = RunStatus::Stoped;
            }
        }
    }

    void Suspend()
    {
        SPINLOCK (mLock)
        {
            if (mStatus == RunStatus::Started)
            {
                mStatus = RunStatus::Suspended;
            }
        }
    }

    void Resume()
    {
        SPINLOCK (mLock)
        {
            if (mStatus == RunStatus::Suspended)
            {
                mStatus = RunStatus::Started;
                EmitOne();
            }
        }
    }

    bool IsEnabled() { return mEnabled.load() ; }
    void SetEnable(bool isEnabled) { mEnabled.store(isEnabled); }

    void Add(ValuePtr obj)
    {
        if(mEnabled.load())
        {
            SPINLOCK (mLock)
            {
                mQueue.push_back(obj);
                EmitOne();
            }
        }
     }

     void AddAfter(ValuePtr pObj, int milliseconds)
     {
         if(mEnabled.load())
         {
             DTimer::ExpireFromNow(boost::posix_time::milliseconds(milliseconds)
                                   , std::bind(&DeferredActionCollection::Add,this, pObj));
         }
     }

     void Remove(ValuePtr obj)
     {
         SPINLOCK (mLock)
         {
             mQueue.erase(std::find(mQueue.begin(),mQueue.end(),obj));
         }
     }

     bool Contains(ValuePtr obj)
     {
         SPINLOCK(mLock)
         {
             return mQueue.end() != std::find(mQueue.begin(),mQueue.end(),obj);
         }

         return false;
     }

private:

     void EmitOne()
     {
         if (RunStatus::Started == mStatus && !mHasTaskInRuning)
         {
             if(0 != mQueue.size())
             {
                 ValuePtr obj;
                 if(DeferredMode::FIFO == mMode)
                 {
                     obj = mQueue.front();
                     mQueue.pop_front();
                 }
                 else
                 {
                     obj = mQueue.back();
                     mQueue.pop_back();
                 }
                 mHasTaskInRuning = true;
                 DTimer::RunNow(std::bind(&DeferredActionCollection::EmitOneHandler,this,obj));
             }
         }//if
     }

     void EmitOneHandler(ValuePtr pObj)
     {
         mAction(pObj);
         SPINLOCK(mLock)
         {
             mHasTaskInRuning = false;
             if (0 != mQueue.size())
             {
                 EmitOne();
             }
         }
     }

private:
    ddeque<ValuePtr> mQueue;
    DSpinLock mLock;
    Action mAction;
    int mStatus;
    bool mHasTaskInRuning;
    std::atomic_bool mEnabled;
    DeferredMode::type mMode;
};
#endif // DEFERREDACTIONCOLLECTION_H
