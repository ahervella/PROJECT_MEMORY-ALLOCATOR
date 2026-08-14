#pragma once

#include "basicPoolMM.h"
#include <stdlib.h>

enum class ALLOCATOR_MODE
{
    DEFAULT,
    BASIC_POOL,
    BIT_MAP
};

inline ALLOCATOR_MODE allocatorMode;

inline std::string getName(ALLOCATOR_MODE mode)
{
    switch (mode)
    {
    case ALLOCATOR_MODE::DEFAULT:
        return "Default";

    case ALLOCATOR_MODE::BASIC_POOL:
        return "Basic Pool";

    case ALLOCATOR_MODE::BIT_MAP:
        return "Bit Map";
    }
}

inline void* customNew(size_t size)
{
    switch (allocatorMode)
    {
    case ALLOCATOR_MODE::DEFAULT:
        return malloc(size);

    case ALLOCATOR_MODE::BASIC_POOL:
        return gBasicPoolMM.allocate(size);

    case ALLOCATOR_MODE::BIT_MAP:
        return gBasicPoolMM.allocate(size);
    }
}

void* operator new(size_t size) { return customNew(size); }
void* operator new[](size_t size) { return customNew(size); }

inline void customDelete(void* pointerToDelete)
{
    switch (allocatorMode)
    {
    case ALLOCATOR_MODE::DEFAULT:
        return free(pointerToDelete);

    case ALLOCATOR_MODE::BASIC_POOL:
        return gBasicPoolMM.free(pointerToDelete);
        ;

    case ALLOCATOR_MODE::BIT_MAP:
        return gBasicPoolMM.free(pointerToDelete);
        ;
    }
}

void operator delete(void* pointerToDelete) noexcept
{
    return customDelete(pointerToDelete);
}

void operator delete[](void* pointerToDelete) noexcept
{
    return customDelete(pointerToDelete);
}
