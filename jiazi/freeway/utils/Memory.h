//
// Created by shgli on 17-8-2.
//

#ifndef ARAGOPROJECT_MEMORY_H
#define ARAGOPROJECT_MEMORY_H

template <typename T>
T* ZeroNew(int32_t count)
{
    T* pAddr = new T[count];
    memset(pAddr, 0, sizeof(T) * count);
    return pAddr;
}
#endif //ARAGOPROJECT_MEMORY_H
