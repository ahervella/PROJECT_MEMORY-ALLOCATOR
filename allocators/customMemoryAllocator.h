#pragma once
#include "iMemoryAllocator.h"
#include "basicPoolAllocator.h"
#include "basicPoolTSAllocator.h"
#include "bitMapAllocator.h"
#include "bitMapTSAllocator.h"
#include <cstddef>

inline void* operator new(size_t size, IMemoryAllocator &allocator)
{
    return allocator.allocate(size);
}

inline void* operator new[](size_t size, IMemoryAllocator &allocator)
{
    return allocator.allocateArray(size);
}

template<class T>
inline void operator delete(void* p, IMemoryAllocator &allocator)
{
    static_cast<T*>(p)->~T();
    allocator.free(p);
}

template<class T>
inline void operator delete[](void* p, IMemoryAllocator &allocator, size_t length )
{
    T* t = static_cast<T*>(p);
    for (size_t i = 0; i < length; i++, t++)
    {
        t->~T();
    }
    allocator.freeArray(p);
}