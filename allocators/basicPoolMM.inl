#include "basicPoolMM.h"
#include "badAllocWithMsg.h"

template<class T>
inline BasicPoolMM<T>::BasicPoolMM()
{
    m_freeStoreHead = 0;
    expandPoolSize();
}

template<class T>
inline BasicPoolMM<T>::~BasicPoolMM() { cleanUp(); }

template<class T>
void* BasicPoolMM<T>::allocate(size_t size)
{
    TestForConcreteClass<T>( size );
    
    if (!m_freeStoreHead)
    {
        expandPoolSize();
    }

    FreeStore* head = m_freeStoreHead;
    m_freeStoreHead = head->next;
    return head;
}

template<class T>
inline void BasicPoolMM<T>::free(void* deleted)
{
    FreeStore* head = static_cast<FreeStore*>(deleted);
    head->next = m_freeStoreHead;
    m_freeStoreHead = head;
}

template<class T>
void BasicPoolMM<T>::expandPoolSize()
{
    size_t size = std::max(sizeof(T), sizeof(FreeStore));

    FreeStore* head = reinterpret_cast<FreeStore*>(malloc(size));
    m_freeStoreHead = head;

    for (int i = 0; i < POOLSIZE; i++)
    {
        head->next = reinterpret_cast<FreeStore*>(malloc(size));
        head = head->next;
    }

    head->next = 0;
}

template<class T>
void BasicPoolMM<T>::cleanUp()
{
    while (m_freeStoreHead)
    {
        FreeStore* toDelete = m_freeStoreHead;
        m_freeStoreHead = toDelete->next;
        std::free(toDelete);
    }
}
