#pragma once
#include "iMemoryManager.h"
#include <complex.h>
#include <sys/types.h>

class BasicPoolMM : public IMemoryManager
{
    struct FreeStore
    {
        FreeStore* next;
    };

    void expandPoolSize();
    void cleanUp();

    FreeStore* m_freeStoreHead;

public:
    BasicPoolMM();
    virtual ~BasicPoolMM();
    virtual void* allocate(size_t);
    virtual void free(void*);
};

inline BasicPoolMM gBasicPoolMM;

#include "basicPoolMM.inl"
