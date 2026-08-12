#pragma once

#include "memoryManager.h"
#include <stdlib.h>

inline void* customNew( size_t size )
{
    if (useCustomAllocator)
    {
        return gMemoryManager.allocate(size);
    }
    return malloc(size);
}

void* operator new(size_t size) { return customNew(size); }
void* operator new[](size_t size) { return customNew(size); }


inline void customDelete(void* pointerToDelete)
{
    if (useCustomAllocator)
    {
        return gMemoryManager.free(pointerToDelete);
    }
    return free(pointerToDelete);
}

void operator delete(void* pointerToDelete) noexcept { return customDelete(pointerToDelete); }
void operator delete[](void* pointerToDelete) noexcept { return customDelete(pointerToDelete); }
