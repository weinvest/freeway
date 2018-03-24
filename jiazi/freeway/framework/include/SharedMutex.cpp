//
// Created by shgli on 17-10-12.
//

#include "framework/freeway/SharedMutex.h"
void SharedMutex::LockShared(LockPtr<void> what, Task* pWho)
{
    if (!mWriter && mWaiters.empty())
    {
        ++mReaders;
        what.Signal(LockPtr::LockType::READ_LOCK, pWho);
    }
    else
    {
        mWaiters.emplace_back(what, pWho, false);
    }
}
/// Unlocks a \c SharedMutex after a previous call to \ref lock_shared().
void SharedMutex::UnlockShared()
{
    --mReaders;
    Wake();
}
/// Lock the \c SharedMutex for exclusive access
///
/// \return a future that becomes ready when no access, shared or exclusive
///         is granted to anyone.
future<> SharedMutex::Lock(LockPtr<void> what, Task* pWho)
{
    if (!mReaders && !mWriter)
    {
        mWriter = true;
        what.Signal(LockPtr::LockType::WRITE_LOCK, pWho);
    }
    else
    {
        mWaiters.emplace_back(what, pWho, true);
    }
}
/// Unlocks a \c SharedMutex after a previous call to \ref lock().
void SharedMutex::Unlock()
{
    mWriter = false;
    Wake();
}

void SharedMutex::Wake()
{
    while (!mWaiters.empty())
    {
        auto& w = mWaiters.front();
        // note: mWriter == false in Wake()
        if (w.ForWrite)
        {
            if (!mReaders)
            {
                mWriter = true;
                w.What.Signal(LockPtr::LockType::WRITE_LOCK, w.Who);
                mWaiters.pop_front();
            }
            break;
        }
        else
        { // for read
            ++mReaders;
            w.What.Signal(LockPtr::LockType::READ_LOCK, w.Who);
            mWaiters.pop_front();
        }
    }
}