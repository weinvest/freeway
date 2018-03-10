#ifndef _DFC_PRODUCTERCONSUMERCOLLECTION_H
#define _DFC_PRODUCTERCONSUMERCOLLECTION_H
#include <functional>
#include "common/DSpinLock.h"
#include "common/Types.h"
template<typename T,typename Ptr = typename T::Ptr, typename QueueT = dvector<Ptr> >
class ProducterConsumerCollection
{
public:
    typedef QueueT Queue;

    void Enqueue(Ptr pProduct)
    {
        SPINLOCK(mLock)
        consist_insert<Ptr>::push_back(mProductQueue, pProduct);
    }

    bool Foreach(std::function<void(Ptr&)> f)
    {
        auto& consumeQueue = GetAllPendings();
        for(auto& pProduct : consumeQueue)
        {
            f(pProduct);
        }
        return consumeQueue.size() != 0;
    }

    Queue& GetAllPendings( void )
    {
        mConsumeQueue.clear();
        {
            SPINLOCK(mLock)
            mConsumeQueue.swap(mProductQueue);
        }
        return mConsumeQueue;
    }

    bool IsEmpty() const
    {
        return 0 == mProductQueue.size();
    }
private:
    Queue mProductQueue;
    Queue mConsumeQueue;
    DSpinLock mLock;
};
#endif // PRODUCTERCONSUMERCOLLECTION_H
