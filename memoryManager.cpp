#include "memoryManager.h"
#include "complex.h"
#include <algorithm>

#define POOLSIZE 32

void MemoryManager::expandPoolSize()
{
    size_t size = std::max(sizeof(Complex), sizeof(FreeStore));

    FreeStore* head = reinterpret_cast<FreeStore*>(malloc(size));
    m_freeStoreHead = head;

    for (int i = 0; i < POOLSIZE; i++)
    {
        head->next = reinterpret_cast<FreeStore*>(malloc(size));
        head = head->next;
    }

    head->next = 0;
}

void MemoryManager::cleanUp()
{
    while (m_freeStoreHead)
    {
        FreeStore* toDelete = m_freeStoreHead;
        m_freeStoreHead = toDelete->next;
        std::free(toDelete);
    }
}