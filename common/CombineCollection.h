#ifndef _DFC_COMBINESUMERCOLLECTION_H
#define _DFC_COMBINESUMERCOLLECTION_H
#include <vector>
#include <boost/function.hpp>
#include <tbb/enumerable_thread_specific.h>
#include "common/DSpinLock.h"
#include "common/Types.h"
template<typename T,typename Ptr = typename T::Ptr, typename C = dvector<Ptr> >
class CombineCollection
{
public:
    typedef tbb::enumerable_thread_specific<C> Queue;
    CombineCollection():mIsEmpty(true){}

    CombineCollection(const CombineCollection&) = delete;


    void Enqueue(const Ptr& pProduct)
    {
        mIsEmpty = false;
        consist_insert<Ptr>::push_back(mQueue.local(), pProduct);
    }

    bool Foreach(boost::function<void(Ptr&)> f)
    {
        if(mIsEmpty)
        {
            return false;
        }

        bool hasObject = false;
        for(auto& localQueue : mQueue)
        {
            for(auto& pProduct : localQueue)
            {
                hasObject = true;
                f(pProduct);
            }
            localQueue.clear();
        }

        mIsEmpty = true;
        return hasObject;
    }

    dvector<Ptr> GetAllPendings( void )
    {
        dvector<Ptr> consumeQueue;
        for(auto& localQueue : mQueue)
        {
            consist_insert<Ptr>::reserve(consumeQueue, consumeQueue.size() + localQueue.size());
            consist_insert<Ptr>::insert(consumeQueue, localQueue.begin(), localQueue.end());
            localQueue.clear();
        }
        mIsEmpty = true;
        return std::move(consumeQueue);
    }


    bool IsEmpty() const { return mIsEmpty; }

private:
    Queue mQueue;
    bool mIsEmpty;
};
#endif // _DFC_COMBINESUMERCOLLECTION_H
