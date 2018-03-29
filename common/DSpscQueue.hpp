//
// Created by shgli on 17-12-13.
//

#ifndef ARAGOPROJECT_DSPSCQUEUE_HPP_H
#define ARAGOPROJECT_DSPSCQUEUE_HPP_H

#include <stdint.h>
#include "common/spsc_queue.hpp"
#include "common/Types.h"

template <typename T>
struct NullTraits
{
    static inline bool IsNull(const T& o) { return o.IsNull(); }
    static inline T Null( void ) { return T::Null(); }
};

template<typename T>
struct NullTraits<T*>
{
    static inline bool IsNull(T* v) { return nullptr == v; }
    static inline T* Null( void ) { return nullptr; }
};

template <typename T>
class DSpscQueue
{
public:
    struct Holder
    {
        T value;
        char* __pading[64-sizeof(T)];
        Holder* next = nullptr;
    };

    DSpscQueue() = default;
    DSpscQueue(const DSpscQueue&) = delete;

    //Init must be called before Push/Pop
    void Init(int32_t capacity)
    {
        mPool = new Holder[capacity];
        (mPool[0].value) = NullTraits<T>::Null();

        mFreeHolders.init(&mPool[0]);

        for(auto iFreeHolder = 1; iFreeHolder < capacity; ++iFreeHolder)
        {
            (mPool[iFreeHolder].value) = NullTraits<T>::Null();
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
        T ret = NullTraits<T>::Null();
        Holder* pLastHolder = nullptr;
        Holder* pHolder = mStoreHolders.pop(&pLastHolder);
        if(nullptr == pHolder)
        {
            if(!NullTraits<T>::IsNull(pLastHolder->value))
            {
                ret = pLastHolder->value;
                (pLastHolder->value) = NullTraits<T>::Null();
            }

            return ret;
        }

        ret = pHolder->value;
        mFreeHolders.push(pHolder);

        if(NullTraits<T>::IsNull(ret))
        {
            return Pop();
        }
        return ret;
    }

    bool Empty() const { return NullTraits<T>::IsNull(mStoreHolders.head->value) && nullptr == mStoreHolders.head->next; }

    void Pop2(Holder* pHolder)
    {
        (pHolder->value) = NullTraits<T>::Null();
        Holder* pLastHolder = nullptr;
        auto pPop = mStoreHolders.pop(&pLastHolder);
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

    Holder* First( void ) const {
        auto pHolder = mStoreHolders.head;
        return pHolder;
    }

    Holder* Next(Holder* pHolder) const { return mStoreHolders.next(pHolder); }

    template<typename CallBack_t>
    void consume_all(CallBack_t cb)
    {
        auto node = Pop();
        while(!NullTraits<T>::IsNull(node))
        {
            cb(node);
            node = Pop();
        }
    }
private:
    spsc_queue<Holder*> mStoreHolders;
    spsc_queue<Holder*> mFreeHolders;
    Holder* mPool;
};

//
//template <typename T>
//class DSpscQueue<T*>
//{
//public:
//    struct Holder
//    {
//        Holder* next = nullptr;
//        char* __pading[64-8];
//        T* value = nullptr;
//    };
//
//    DSpscQueue() = default;
//    DSpscQueue(const DSpscQueue&) = delete;
//
//    //Init must be called before Push/Pop
//    void Init(int32_t capacity)
//    {
//        mPool = new Holder[capacity];
//        mPool[0].value = nullptr;
//
//        mFreeHolders.init(&mPool[0]);
//
//        for(auto iFreeHolder = 1; iFreeHolder < capacity; ++iFreeHolder)
//        {
//            mPool[iFreeHolder].value = nullptr;
//            mFreeHolders.push(&mPool[iFreeHolder]);
//        }
//
//        mStoreHolders.init(mFreeHolders.pop());
//    }
//
//    bool Push(T* value)
//    {
//        Holder* pLastFreeHolder = nullptr;
//        Holder* pHolder = mFreeHolders.pop(&pLastFreeHolder);
//
//        if(LIKELY(nullptr != pHolder))
//        {
//            pHolder->value = value;
//            mStoreHolders.push(pHolder);
//            return true;
//        }
//
//        return false;
//    }
//
//    T* Pop( void )
//    {
//        T* ret = nullptr;
//        Holder* pLastHolder = nullptr;
//        Holder* pHolder = mStoreHolders.pop(&pLastHolder);
//        if(nullptr == pHolder)
//        {
//            ret = pLastHolder->value;
//            pLastHolder->value = nullptr;
//        } else{
//            ret = pHolder->value;
//        }
//
//        return ret;
//    }
//
//    bool Empty() const { return nullptr == mStoreHolders.head->value && nullptr == mStoreHolders.head->next; }
//
//    template<typename CallBack_t>
//    void consume_all(CallBack_t cb)
//    {
//        auto node = Pop();
//        while(LIKELY(nullptr != node))
//        {
//            cb(node);
//            node = Pop();
//        }
//    }
//private:
//    spsc_queue<Holder*> mStoreHolders;
//    spsc_queue<Holder*> mFreeHolders;
//    Holder* mPool;
//};
#endif //ARAGOPROJECT_DSPSCQUEUE_HPP_H
