#pragma once
#include "bitMapAllocator.h"
#include <mutex>

template <class T>
class BitMapTSAllocator : public BitMapAllocator<T>
{
public:
    BitMapTSAllocator<T>(){}
    ~BitMapTSAllocator<T>(){}
    
    void* allocate(size_t size) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BitMapAllocator<T>::allocate(size);
    }

    void* allocateArray(size_t size) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BitMapAllocator<T>::allocateArray(size);
    }

    void free(void* p) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BitMapAllocator<T>::free(p);
    }

    void freeArray(void* p) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BitMapAllocator<T>::freeArray(p);
    }

private:
    std::mutex m_lock;
};
