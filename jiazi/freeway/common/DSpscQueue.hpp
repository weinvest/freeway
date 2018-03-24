//
// Created by shgli on 17-12-13.
//

#ifndef ARAGOPROJECT_DSPSCQUEUE_HPP_H
#define ARAGOPROJECT_DSPSCQUEUE_HPP_H

#include <stdint.h>
#include "common/spsc_queue.hpp"

template <typename T>
class DSpscQueue
{
public:
    struct Holder
    {
        Holder* next = nullptr;
        T value;
    };

    DSpscQueue() {mPool = nullptr;};
    DSpscQueue(int32_t capacity, T nullValue) {Init(capacity, nullValue);} 
    DSpscQueue(const DSpscQueue&) = delete;


    //Init must be called before Push/Pop
    void Init(int32_t capacity, T nullValue)
    {
        assert(capacity > 2);

        mNullValue = nullValue;
        mPool = new Holder[capacity];
        mPool[0].value = mNullValue;
        mPool[1].value = mNullValue;

        mStoreHolders.init(&mPool[0]);
        mFreeHolders.init(&mPool[1]);
        for(auto iFreeHolder = 2; iFreeHolder < capacity; ++iFreeHolder)
        {
            mPool[iFreeHolder].value = mNullValue;
            mFreeHolders.push(&mPool[iFreeHolder]);
        }
    }

    ~DSpscQueue( void )
    {
        delete []mPool;
    }

    bool Push(T value)
    {
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
        return ret;
    }

    template<typename CallBack_t>
    void consume_all(CallBack_t cb)
    {
        auto Node = Pop();
        while(Node != mNullValue)
        {
            cb(Node);
            Node = Pop();
        }
    }
private:
    spsc_queue<Holder*> mStoreHolders;
    spsc_queue<Holder*> mFreeHolders;
    Holder* mPool;
    T mNullValue;
};
#endif //ARAGOPROJECT_DSPSCQUEUE_HPP_H
