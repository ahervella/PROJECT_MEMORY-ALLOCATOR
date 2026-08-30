#include "BasicPoolAllocator.h"
#include "BadAllocWithMsg.h"

template<class T>
inline BasicPoolAllocator<T>::BasicPoolAllocator()
{
    m_freeStoreHead = 0;
    expandPoolSize();
}

template<class T>
inline BasicPoolAllocator<T>::~BasicPoolAllocator() { cleanUp(); }

template<class T>
void* BasicPoolAllocator<T>::allocate(size_t size)
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
inline void BasicPoolAllocator<T>::free(void* deleted)
{
    FreeStore* head = static_cast<FreeStore*>(deleted);
    head->next = m_freeStoreHead;
    m_freeStoreHead = head;
}

template<class T>
void BasicPoolAllocator<T>::expandPoolSize()
{
    size_t size = std::max(sizeof(T), sizeof(FreeStore));

    void* p = malloc(size);
    if (!p)
    {
       throw BadAllocWithMsg(std::string("Error, malloc returned null pointer!")); 
    }
    
    FreeStore* head = static_cast<FreeStore*>(p);
    m_freeStoreHead = head;

    //already did head, so - 1
    for (int i = 0; i < POOLSIZE - 1; i++)
    {
        void* p = malloc(size);
        if (!p)
        {
            throw BadAllocWithMsg(std::string("Error, malloc returned null pointer!"));
        }
        
        head->next = static_cast<FreeStore*>(p);
        head = head->next;
    }

    head->next = 0;
}

template<class T>
void BasicPoolAllocator<T>::cleanUp()
{
    while (m_freeStoreHead)
    {
        FreeStore* toDelete = m_freeStoreHead;
        m_freeStoreHead = toDelete->next;
        std::free(toDelete);
    }
}
