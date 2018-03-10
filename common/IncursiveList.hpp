//
// Created by shgli on 17-11-30.
//

#ifndef ARAGOPROJECT_INCURSIVE_LIST_HPP_H
#define ARAGOPROJECT_INCURSIVE_LIST_HPP_H

template <typename T>
class IncursiveList
{
public:
    using reference = typename std::remove_pointer<T>::type;
    class Iterator
    {
    public:
        Iterator(T obj):mObject(obj){}

        T operator-> () { return mObject; }
        T operator* () { return mObject; }

        const T operator-> () const { return mObject; }
        const T operator* () const { return mObject; }

        Iterator& operator++() { mObject = mObject->next; return *this; }
        Iterator& operator--() { mObject = mObject->prev; return *this; }

        bool operator==(const Iterator& o) const { return mObject == o.mObject; }
        bool operator!=(const Iterator& o) const { return mObject != o.mObject; }
    private:
        T mObject;
    };

    class RIterator
    {
    public:
        RIterator(T obj):mObject(obj){}

        T operator-> () { return mObject; }
        T operator* () { return mObject; }

        const T operator-> () const { return mObject; }
        const T operator* () const { return mObject; }

        RIterator& operator--() { mObject = mObject->next; return *this; }
        RIterator& operator++() { mObject = mObject->prev; return *this; }

        bool operator==(const Iterator& o) const { return mObject == o.mObject; }
        bool operator!=(const RIterator& o) const { return mObject != o.mObject; }
    private:
        T mObject;
    };

    IncursiveList()
        :mSize(0)
        ,mHead(nullptr)
        ,mTail(nullptr)
    {}

    void push_back(T obj)
    {
        ++mSize;
        if(1 == mSize)
        {
            mHead = mTail = obj;
            obj->prev = obj->next = nullptr;
        }
        else
        {
            mTail->next = obj;
            obj->prev = mTail;
            mTail = obj;
        }
    }

    void push_front(T obj)
    {
        ++mSize;
        if(1 == mSize)
        {
            mHead = mTail = obj;
            obj->prev = obj->next = nullptr;
        }
        else
        {
            obj->next = mHead;
            mHead->prev = obj;
            mHead = obj;
        }
    }

    void insert_after(T prev, T obj)
    {
        if(nullptr == prev)
        {
            push_front(obj);
        }
        else
        {
            ++mSize;
            obj->next = prev->next;
            obj->prev = prev;
            prev->next = obj;
            if(nullptr != obj->next)
            {
                obj->next->prev = obj;
            }
        }
    }

    void insert_before(T next, T obj)
    {
        if(nullptr == next)
        {
            push_back(obj);
        }
        else
        {
            ++mSize;
            obj->prev = next->prev;
            obj->next = next;
            next->prev = obj;
            if(nullptr != obj->prev)
            {
                obj->prev->next = obj;
            }
        }
    }

    void erase(Iterator& it)
    {
        erase(*it);
    }

    void erase(T obj)
    {
        --mSize;
        auto pPrev = obj->prev;
        auto pNext = obj->next;
        if(nullptr != pPrev)
        {
            pPrev->next = pNext;
        }
        else
        {
            mHead = pNext;
        }

        if(nullptr != pNext)
        {
            pNext->prev = pPrev;
        }
        else
        {
            mTail = pPrev;
        }
        obj->prev = obj->next = nullptr;
    }

    size_t size( void ) const { return mSize; }
    bool empty( void ) const { return 0 == mSize; }

    T front( void ) { return mHead; }
    T back( void ) { return mTail; }

    const T front( void ) const { return mHead; }
    const T back( void ) const { return mTail; }

    Iterator begin( void ) { return Iterator(mHead); }
    Iterator end( void ) { return Iterator(nullptr); }

    RIterator rbegin( void ) { return RIterator(mTail); }
    RIterator rend( void ) { return RIterator(nullptr); }

    T operator[](int32_t idx)
    {
        auto data = mHead;
        while(idx > 0)
        {
            data = data->next;
            --idx;
        }

        return data;
    }
private:
    size_t mSize;
    T mHead;
    T mTail;
};

#endif //ARAGOPROJECT_INCURSIVE_LIST_HPP_H
