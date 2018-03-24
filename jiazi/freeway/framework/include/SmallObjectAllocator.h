//
// Created by shgli on 17-10-11.
//

#ifndef ARAGOPROJECT_SMALLOBJECTALLOCATOR_H
#define ARAGOPROJECT_SMALLOBJECTALLOCATOR_H

#include <memory>
#include <array>
#include "common/spsc_queue.hpp"

class SmallObjectAllocatorImpl
{
    struct SmallObject;
    std::array<spsc_queue<SmallObject*>, 16> mFreeLists;
    uint8_t* mPool;
public:
    SmallObjectAllocatorImpl( void );
    ~SmallObjectAllocatorImpl( void );

    uint8_t* allocate(size_t size);
    void deallocate(uint8_t* ptr,size_t size);

    inline static constexpr size_t Align(size_t v, size_t align)
    {
        return (v) & ~(align - 1);
    }
};

extern SmallObjectAllocatorImpl* GetAllocator( void );
template <typename T>
class SmallObjectAllocator: public std::allocator<T>
{
public:
    typedef std::allocator<T> base_type;
    using value_type = typename base_type::value_type;
    using pointer = typename base_type::pointer;
    using size_type = typename base_type::size_type;

    template<typename U>
    struct rebind
    {
        using other = SmallObjectAllocator<U>;
    };


    SmallObjectAllocator(SmallObjectAllocatorImpl* pImpl) throw()
    :mImpl(pImpl)
    {}

    SmallObjectAllocator(const SmallObjectAllocator& o)  throw()
    :base_type(o)
    ,mImpl(o.mImpl){}

    template <typename U>
    SmallObjectAllocator(const U&o) throw()
    :mImpl(o.mImpl)
    {}

    value_type* allocate(size_type size)
    {
        auto pAllocImpl = mImpl;
        if(nullptr == pAllocImpl)
        {
            return base_type::allocate(size);
        }

        return (value_type*)pAllocImpl->allocate(size * sizeof(T));
    }

    void deallocate(value_type* ptr, size_type size)
    {
        auto pAllocImpl = mImpl;
        if(nullptr == pAllocImpl)
        {
            return base_type::deallocate(ptr, size);
        }

        return pAllocImpl->deallocate((uint8_t*)ptr, size);
    }

private:
    SmallObjectAllocatorImpl* mImpl;
    template <typename U> friend class SmallObjectAllocator;
};


#endif //ARAGOPROJECT_SMALLOBJECTALLOCATOR_H
