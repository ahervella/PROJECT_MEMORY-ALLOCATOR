#pragma once
#include <cstddef>

class IMemoryManager
{
public:
    virtual void* allocate(std::size_t) = 0;
    virtual void free(void*) = 0;
};
