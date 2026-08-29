#pragma once
#include "iMemoryManager.h"
#include "basicPoolMM.h"
#include "basicPoolTSMM.h"
#include "bitMapMM.h"
#include "bitMapTSMM.h"
#include <cstddef>

inline void* operator new(size_t size, IMemoryManager &allocator)
{
    return allocator.allocate(size);
}

inline void* operator new[](size_t size, IMemoryManager &allocator)
{
    return allocator.allocateArray(size);
}

template<class T>
inline void operator delete(void* p, IMemoryManager &allocator)
{
    static_cast<T*>(p)->~T();
    allocator.free(p);
}

template<class T>
inline void operator delete[](void* p, IMemoryManager &allocator, size_t length )
{
    T* t = static_cast<T*>(p);
    for (size_t i = 0; i < length; i++, t++)
    {
        t->~T();
    }
    allocator.freeArray(p);
}