/*
#include "memoryManager.h"
#include <stdlib.h>

inline void* Complex::operator new(size_t size )
{
    return customNew(size);
}

inline void* Complex::operator new[](size_t size)
{
    return customNew(size);
}

inline void* Complex::customNew( size_t size )
{
    if (Complex::customAllocator)
    {
        return gMemoryManager.allocate(size);
    }
    return malloc(size);
}

inline void Complex::operator delete(void* pointerToDelete)
{
    return customDelete(pointerToDelete);
}

inline void Complex::operator delete[](void* pointerToDelete)
{
    return customDelete(pointerToDelete);
}

inline void Complex::customDelete(void* pointerToDelete)
{
    if (Complex::customAllocator)
    {
        return gMemoryManager.free(pointerToDelete);
    }
    return free(pointerToDelete);
}
    */