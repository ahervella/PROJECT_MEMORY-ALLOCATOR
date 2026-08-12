#pragma once
#include <cstddef>
#include <sys/types.h>

class IMemoryManager
{
public:
    virtual void* allocate(std::size_t) = 0;
    virtual void free(void*) = 0;
};

class MemoryManager : public IMemoryManager
{
    struct FreeStore
    {
        FreeStore* next;
    };

    void expandPoolSize();
    void cleanUp();

    FreeStore* m_freeStoreHead;

public:
    MemoryManager();
    virtual ~MemoryManager();
    virtual void* allocate(size_t);
    virtual void free(void*);
};

inline MemoryManager gMemoryManager;
inline bool useCustomAllocator;

#include "memoryManager.inl"