#pragma once
#include "basicPoolMM.h"
#include <mutex>

template <class T>
class BasicPoolTSMM : public BasicPoolMM<T>
{
public:
    virtual void* allocate(size_t size)
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BasicPoolMM<T>::allocate(size);
    }

    virtual void* allocateArray(size_t size)
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BasicPoolMM<T>::allocateArray(size);
    }

    virtual void free(void* p)
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BasicPoolMM<T>::free(p);
    }

    virtual void freeArray(void* p)
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BasicPoolMM<T>::freeArray(p);
    }

private:
    std::mutex m_lock;
};
