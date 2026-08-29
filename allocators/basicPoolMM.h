#pragma once
#include "iMemoryManager.h"
#include <sys/types.h>
#include <cstdlib>

#define POOLSIZE 32

template<class T>
class BasicPoolMM : public IMemoryManager
{
private:
    struct FreeStore
    {
        FreeStore* next;
    };

    virtual void expandPoolSize();
    virtual void cleanUp();

    FreeStore* m_freeStoreHead;

public:
    BasicPoolMM<T>();
    virtual ~BasicPoolMM<T>();
    virtual void* allocate(size_t);
    virtual void* allocateArray(size_t size)
    {
        //cannot allocate object array
        return std::malloc(size);
    }

    virtual void free(void*);
    virtual void freeArray(void* p)
    {
        // cannot allocate object array
        std::free(p);
    }
};


#include "basicPoolMM.inl"
