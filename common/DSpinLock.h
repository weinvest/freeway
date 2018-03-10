#ifndef _DSPINLOCK_H
#define _DSPINLOCK_H
#include <boost/smart_ptr/detail/spinlock.hpp>

typedef boost::detail::spinlock SpinLock;

struct DSpinLock:public SpinLock
{
    DSpinLock(){ v_ = 0;}
};

#define SPINLOCK(sp) boost::detail::spinlock::scoped_lock __lock(sp);
#endif
