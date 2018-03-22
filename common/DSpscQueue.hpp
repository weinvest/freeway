//
// Created by shgli on 17-12-13.
//

#ifndef ARAGOPROJECT_DSPSCQUEUE_HPP_H
#define ARAGOPROJECT_DSPSCQUEUE_HPP_H

#include <stdint.h>
#include "common/spsc_queue.hpp"
#include "common/Types.h"
template <typename T>
class DSpscQueue
{
public:
    struct Holder
    {
        Holder* next = nullptr;
        T value;
    };

    DSpscQueue() = default;
    DSpscQueue(const DSpscQueue&) = delete;

    //Init must be called before Push/Pop
    void Init(int32_t capacity, T nullValue)
    {
        mPool = new Holder[capacity];
        mNullValue = nullValue;
        mPool[0].value = mNullValue;

        mFreeHolders.init(&mPool[0]);

        for(auto iFreeHolder = 1; iFreeHolder < capacity; ++iFreeHolder)
        {
            mPool[iFreeHolder].value = mNullValue;
            mFreeHolders.push(&mPool[iFreeHolder]);
        }

        mStoreHolders.init(mFreeHolders.pop());
    }

    bool Push(T value)
    {
//        if(UNLIKELY(mFirstPush))
//        {
//            mFirstPush = false;
//            mStoreHolders.head->value = value;
//            return true;
//        }

        Holder* pLastFreeHolder = nullptr;
        Holder* pHolder = mFreeHolders.pop(&pLastFreeHolder);
//     assert(nullptr != pHolder);

        if(nullptr != pHolder)
        {
            pHolder->value = value;
            mStoreHolders.push(pHolder);
            return true;
        }

        return false;
    }

    T Pop( void )
    {
        T ret = mNullValue;
        Holder* pLastHolder = nullptr;
        Holder* pHolder = mStoreHolders.pop(&pLastHolder);
        if(nullptr == pHolder)
        {
            if(mNullValue != pLastHolder->value)
            {
                ret = pLastHolder->value;
                pLastHolder->value = mNullValue;
            }

            return ret;
        }

        ret = pHolder->value;
        mFreeHolders.push(pHolder);

        if(ret == mNullValue)
        {
            return Pop();
        }
        return ret;
    }

    bool Empty() { return mNullValue == mStoreHolders.head->value && mStoreHolders.empty(); }

    const T& Null() { return mNullValue; }

    void Pop2(Holder* pHolder)
    {
        pHolder->value = mNullValue;

        auto pPop = mStoreHolders.pop();
        while(nullptr != pPop && pPop != pHolder)
        {
            mFreeHolders.push(pPop);
            pPop = mStoreHolders.pop();
        }

        if(pPop == pHolder)
        {
            mFreeHolders.push(pHolder);
        }
    }

    Holder* First( void ) { auto pHolder = mStoreHolders.head; return mNullValue == pHolder->value ? pHolder->next : pHolder; }

    Holder* Next(Holder* pHolder) { return mStoreHolders.next(pHolder); }

    bool Empty(Holder* pHoler) { return mNullValue == pHoler->next; }
    template<typename CallBack_t>
    void consume_all(CallBack_t cb)
    {
        auto node = Pop();
        while(node != mNullValue)
        {
            cb(node);
            node = Pop();
        }
    }
private:
    spsc_queue<Holder*> mStoreHolders;
    spsc_queue<Holder*> mFreeHolders;
    Holder* mPool;
    T mNullValue;
//    bool mFirstPush{true};
};
#endif //ARAGOPROJECT_DSPSCQUEUE_HPP_H
