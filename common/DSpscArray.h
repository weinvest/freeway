//
// Created by 李书淦 on 2018/2/20.
//

#ifndef FREEWAY_DSPSCARRAY_H
#define FREEWAY_DSPSCARRAY_H

#include <stddef.h>

template <typename T>
class DSpscArray
{
public:
    DSpscArray(size_t capacity)
    :mData(new T[capacity])
    ,mCapacity(capacity)
    {
    }

    ~DSpscArray() { delete [] mData; }

    void push(T&& data)
    {
        mData[mWritePos++%mCapacity] = std::move(data);
    }

    T front( void )
    {
        return mData[mReadPos%mCapacity];
    }

    void pop( void )
    {
        mReadPos++;
    }

    size_t size( void )
    {
        return mWritePos - mReadPos - 1;
    }

    bool empty( void )
    {
        return 0 == size();
    }
private:
    T* mData{nullptr};
    size_t mCapacity;
    size_t mWritePos{1};
    char __pading[64-sizeof(size_t)];
    size_t mReadPos{0};
};
#endif //FREEWAY_DSPSCARRAY_H
