//
// Created by 李书淦 on 2018/2/20.
//

#ifndef FREEWAY_DSPSCARRAY_H
#define FREEWAY_DSPSCARRAY_H

#include <stddef.h>
#include <atomic>
template <typename T>
struct CallTraits
{
    typedef T&& PushParamType;
    typedef T& FirstReturnType;
};

template <typename T>
struct CallTraits<T*>
{
    typedef T* PushParamType;
    typedef T* FirstReturnType;
};

#define DEFINE_BUILDIN_CALLTRAITS(T) template <>\
struct CallTraits<T>\
{\
    typedef T PushParamType;\
    typedef T FirstReturnType;\
};

DEFINE_BUILDIN_CALLTRAITS(bool)
DEFINE_BUILDIN_CALLTRAITS(char)
DEFINE_BUILDIN_CALLTRAITS(int16_t)
DEFINE_BUILDIN_CALLTRAITS(int32_t)
DEFINE_BUILDIN_CALLTRAITS(int64_t)
DEFINE_BUILDIN_CALLTRAITS(uint16_t)
DEFINE_BUILDIN_CALLTRAITS(uint32_t)
DEFINE_BUILDIN_CALLTRAITS(uint64_t)
DEFINE_BUILDIN_CALLTRAITS(float)
DEFINE_BUILDIN_CALLTRAITS(double)

//template <typename T, typename WT=std::atomic_int, typename RT=std::atomic_int>
template <typename T, typename WT=int32_t, typename RT=int32_t>
class DSpscArray
{
public:
    DSpscArray()
    :mData(nullptr)
    ,mCapacity(0)
    {
    }

    void Init(int32_t capacity)
    {
        mData = new T[capacity];
        mCapacity = capacity;
    }

    ~DSpscArray() { delete [] mData; }

    void Push(typename CallTraits<T>::PushParamType data)
    {
        mData[mWritePos%mCapacity] = data;
        mWritePos++;
    }

    typename CallTraits<T>::FirstReturnType First( void )
    {
        return mData[mReadPos%mCapacity];
    }

    typename CallTraits<T>::FirstReturnType First(int32_t k)
    {
        return mData[(mReadPos+k)%mCapacity];
    }

    bool Valid(int32_t k)
    {
        return mWritePos - mReadPos - k > 0;
    }

    void Pop( void )
    {
        mReadPos++;
    }

    void Skip(int32_t k)
    {
        mReadPos += k;
    }

    int32_t Size( void )
    {
        return mWritePos - mReadPos;
    }

    bool Empty( void )
    {
        return 0 == Size();
    }

    template<typename CallBack_t>
    void consume_all(CallBack_t cb)
    {
        int32_t writePos = mWritePos;
        int32_t readPos = mReadPos;
        while(readPos < writePos)
        {
            cb(mData[readPos%mCapacity]);
            ++readPos;
        }

        mReadPos = readPos;
    }
private:
    T* mData{nullptr};
    int32_t mCapacity;
    char __pading[64-sizeof(int32_t)];
    WT mWritePos{0};
    char __pading1[64-sizeof(WT)];
    RT mReadPos{0};
};
#endif //FREEWAY_DSPSCARRAY_H
