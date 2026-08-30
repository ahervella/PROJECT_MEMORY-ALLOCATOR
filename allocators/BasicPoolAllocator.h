#pragma once
#include "IMemoryAllocator.h"
#include <sys/types.h>
#include <cstdlib>

#define POOLSIZE 32

template<class T>
class BasicPoolAllocator : public IMemoryAllocator
{
private:
    struct FreeStore
    {
        FreeStore* next;
    };

    void expandPoolSize();
    void cleanUp();

    FreeStore* m_freeStoreHead;

public:
    BasicPoolAllocator<T>();
    ~BasicPoolAllocator<T>();
    void* allocate(size_t) override;
    void* allocateArray(size_t size) override
    {
        // cannot allocate object array due to non-adjacent nodes
        return std::malloc(size);
    }

    void free(void*) override;
    void freeArray(void* p) override
    {
        // cannot allocate object array
        std::free(p);
    }
};


#include "BasicPoolAllocator.inl"
