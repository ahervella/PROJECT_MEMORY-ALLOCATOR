#pragma once
#include <cstddef>

class IMemoryManager
{
public:
    IMemoryManager(){};
    virtual ~IMemoryManager(){};
    virtual void* allocate(size_t) = 0;
    virtual void* allocateArray(size_t) = 0;
    virtual void free(void*) = 0;
    virtual void freeArray(void*) = 0;
};

