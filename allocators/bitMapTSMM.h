#pragma once
#include "bitMapMM.h"
#include <mutex>

template <class T>
class BitMapTSMM : public BitMapMM<T>
{
public:
    virtual void* allocate(size_t size)
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BitMapMM<T>::allocate(size);
    }

    virtual void* allocateArray(size_t size)
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BitMapMM<T>::allocateArray(size);
    }

    virtual void free(void* p)
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BitMapMM<T>::free(p);
    }

    virtual void freeArray(void* p)
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BitMapMM<T>::freeArray(p);
    }

private:
    std::mutex m_lock;
};
