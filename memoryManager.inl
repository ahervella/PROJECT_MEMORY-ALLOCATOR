inline MemoryManager::MemoryManager()
{
    m_freeStoreHead = 0;
    expandPoolSize();
}

inline MemoryManager::~MemoryManager()
{
    cleanUp();
}

inline void* MemoryManager::allocate(size_t)
{
    if (!m_freeStoreHead)
    {
        expandPoolSize();
    }

    FreeStore* head = m_freeStoreHead;
    m_freeStoreHead = head->next;
    return head;
}

inline void MemoryManager::free(void* deleted)
{
    FreeStore* head = static_cast<FreeStore*>(deleted);
    head->next = m_freeStoreHead;
    m_freeStoreHead = head;
}