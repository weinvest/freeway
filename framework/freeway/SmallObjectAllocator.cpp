//
// Created by shgli on 17-10-11.
//
#include <cassert>
#include "framework/freeway/SmallObjectAllocator.h"

struct SmallObjectAllocatorImpl::SmallObject
{
    SmallObject* next;
};

static constexpr int32_t MAX_POOL_SIZE = 1 << 20;
SmallObjectAllocatorImpl::SmallObjectAllocatorImpl( void )
:mPool(new uint8_t[MAX_POOL_SIZE])
{
    auto nSliceSize = (8 + mFreeLists.size()*8) * mFreeLists.size() / 2;
    auto n = MAX_POOL_SIZE / nSliceSize;
    uint8_t *currAddress = mPool;
    auto objectSize = 0;
    for(size_t iSlot = 0; iSlot < mFreeLists.size(); ++iSlot)
    {
        objectSize += 8;
        mFreeLists[iSlot].init((SmallObject*)currAddress);
        for(size_t nCount = 0; nCount < n; ++nCount, currAddress += objectSize)
        {
            mFreeLists[iSlot].push((SmallObject*)(currAddress + objectSize));
        }

        mFreeLists[iSlot].tail->next = nullptr;
        currAddress += objectSize;
    }
}

SmallObjectAllocatorImpl::~SmallObjectAllocatorImpl( void )
{
    delete[] mPool;
}

uint8_t* SmallObjectAllocatorImpl::allocate(size_t size)
{
    auto slotIdx = Align(size, 8) >> 3;
    assert(slotIdx < mFreeLists.size());

    auto pObject = mFreeLists[slotIdx].pop();
    return (uint8_t*)pObject;
}

void SmallObjectAllocatorImpl::deallocate(uint8_t* ptr, size_t size)
{
    auto slotIdx = Align(size, 8) >> 3;
    assert(slotIdx < mFreeLists.size());

    SmallObject* pObject = (SmallObject*)ptr;
    pObject->next = nullptr;

    mFreeLists[slotIdx].push(pObject);
}



