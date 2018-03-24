//
// Created by shgli on 17-10-12.
//

#ifndef ARAGOPROJECT_SHAREDMUTEX_H
#define ARAGOPROJECT_SHAREDMUTEX_H
/*
#include "common/Types.h"
class SharedMutex
{
    unsigned mReaders = 0;
    bool mWriter = false;
    struct Waiter
    {
        Waiter(LockPtr<void>* what, Task* who, bool forWrite) : What(*what), Who(who), ForWrite(forWrite) {}
        LockPtr<void>& What;
        Task* Who;
        bool ForWrite;
    };

    boost::circular_buffer<Waiter> mWaiters;
public:
    SharedMutex() = default;
    SharedMutex(SharedMutex&&) = default;
    SharedMutex& operator=(SharedMutex&&) = default;
    SharedMutex(const SharedMutex&) = delete;
    void operator=(const SharedMutex&) = delete;

    void LockShared(LockPtr<void> what, Task* pWho);
    void UnlockShared();

    void Lock(LockPtr<void> what, Task* pWho);
    void Unlock();
private:
    void Wake();
};
*/

#endif //ARAGOPROJECT_SHAREDMUTEX_H
