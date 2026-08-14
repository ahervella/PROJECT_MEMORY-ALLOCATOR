inline BasicPoolMM::BasicPoolMM()
{
    m_freeStoreHead = 0;
    expandPoolSize();
}

inline BasicPoolMM::~BasicPoolMM() { cleanUp(); }

inline void* BasicPoolMM::allocate(size_t)
{
    if (!m_freeStoreHead)
    {
        expandPoolSize();
    }

    FreeStore* head = m_freeStoreHead;
    m_freeStoreHead = head->next;
    return head;
}

inline void BasicPoolMM::free(void* deleted)
{
    FreeStore* head = static_cast<FreeStore*>(deleted);
    head->next = m_freeStoreHead;
    m_freeStoreHead = head;
}
