//
// Created by shgli on 17-11-30.
//

#ifndef ARAGOPROJECT_WINDOWEDMAP_H
#define ARAGOPROJECT_WINDOWEDMAP_H

#include "common/IncursiveList.hpp"
template <typename T>
class WindowedMap
{
public:
    struct Holder
    {
        Holder* prev = nullptr;
        Holder* next = nullptr;
        int32_t first = gNullValue;
        T second = T();
    };

    class Iterator
    {
    public:
        Iterator(Holder* pData)
                :mData(pData)
        {}

        Holder* operator->() { return mData; }
        Holder& operator* () { return *mData; }

        const Holder* operator->() const { return mData; }
        const Holder& operator* () const { return *mData; }

        Iterator& operator++() { mData = mData->next; return *this; }
        Iterator& operator--() { mData = mData->prev; return *this; }

        bool operator==(const Iterator& other) const { return mData == other.mData; }
        bool operator!=(const Iterator& other) const { return mData != other.mData; }
    private:
        Holder* mData;
    };

    class RIterator
    {
    public:
        RIterator(Holder* pData)
                :mData(pData)
        {}

        Holder* operator->() { return mData; }
        Holder& operator* () { return *mData; }

        const Holder* operator->() const { return mData; }
        const Holder& operator* () const { return *mData; }

        RIterator& operator--() { mData = mData->next; return *this; }
        RIterator& operator++() { mData = mData->prev; return *this; }

        bool operator==(const RIterator& other) const { return mData == other.mData; }
        bool operator!=(const RIterator& other) const { return mData != other.mData; }
    private:
        Holder* mData;
    };

    WindowedMap(int32_t windowSize)
            :mDatas(new Holder[windowSize])
            ,mWindowSize(windowSize)
    {
    }

    size_t GetWindowSize( void ) const { return mWindowSize; }

    size_t size( void ) const { return mLinkedData.size(); }

    bool count(int32_t key)
    {
        size_t slotIdx = key % mWindowSize;
        auto pSlot = mDatas + slotIdx;
        if(!IsInList(pSlot))
        {
            return 0;
        }

        return 1;
    }

    bool insert(int32_t key, T data)
    {
        size_t slotIdx = key % mWindowSize;
        auto pSlot = mDatas + slotIdx;
        if(!IsInList(pSlot))
        {
            pSlot->second = data;
            Insert2List(key, pSlot);
            return true;
        }

        return false;
    }

    Iterator find(int32_t key)
    {
        size_t slotIdx = key % mWindowSize;
        auto pSlot = mDatas + slotIdx;
        if(!IsInList(pSlot))
        {
            return Iterator(nullptr);
        }

        return Iterator(pSlot);
    }

    T& operator[](int32_t key)
    {
        size_t slotIdx = key % mWindowSize;
        auto pSlot = mDatas + slotIdx;
        if(!IsInList(pSlot))
        {
            Insert2List(key, pSlot);
        }

        return pSlot->second;
    }

    void erase(int32_t key)
    {
        size_t slotIdx = key % mWindowSize;
        auto pSlot = mDatas + slotIdx;
        if(IsInList(pSlot))
        {
            mLinkedData.erase(pSlot);
            pSlot->first = gNullValue;
        }
    }

    Iterator begin() { return Iterator(mLinkedData.front()); }
    Iterator end() { return Iterator(nullptr); }

    RIterator rbegin() { return RIterator(mLinkedData.back()); }
    RIterator rend() { return RIterator(nullptr); }

    const Iterator begin() const { return Iterator(mLinkedData.front()); }
    const Iterator end() const { return Iterator(nullptr); }

    const RIterator rbegin() const { return RIterator(mLinkedData.back()); }
    const RIterator rend() const { return RIterator(nullptr); }
private:
    bool IsInList(Holder* pSlot) { return gNullValue != pSlot->first; }
    void Insert2List(int32_t key, Holder* pSlot)
    {
        pSlot->first = key;
        if(mLinkedData.empty())
        {
            mLinkedData.push_back(pSlot);
            return;
        }

        auto firstKey = mLinkedData.front()->first;
        auto lastKey =  mLinkedData.back()->first;
        if(key < firstKey)
        {
            mLinkedData.push_front(pSlot);
        }
        else if(key > lastKey)
        {
            mLinkedData.push_back(pSlot);
        }
        else if((key - firstKey) < (lastKey - key))
        {
            int32_t tempKey = key - 1;
            while(gNullValue == mDatas[(tempKey + mWindowSize) % mWindowSize].first)
            {
                --tempKey;
            }

            mLinkedData.insert_after(&mDatas[(tempKey + mWindowSize) % mWindowSize], pSlot);
        }
        else
        {
            int32_t tempKey = key + 1;
            while(gNullValue == mDatas[(tempKey + mWindowSize) % mWindowSize].first)
            {
                ++tempKey;
            }

            mLinkedData.insert_before(&mDatas[(tempKey + mWindowSize) % mWindowSize], pSlot);
        }
    }
    static const int32_t gNullValue = (1<<31);

    Holder* mDatas;
    IncursiveList<Holder*> mLinkedData;
    int32_t mWindowSize;
};


#endif //ARAGOPROJECT_WINDOWEDMAP_H
