#pragma once
#include "bitMapMM.h"
#include <mutex>

template <class T>
class BitMapTSMM : public BitMapMM<T>
{
public:
    BitMapTSMM<T>(){}
    ~BitMapTSMM<T>(){}
    
    void* allocate(size_t size) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BitMapMM<T>::allocate(size);
    }

    void* allocateArray(size_t size) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BitMapMM<T>::allocateArray(size);
    }

    void free(void* p) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BitMapMM<T>::free(p);
    }

    void freeArray(void* p) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BitMapMM<T>::freeArray(p);
    }

private:
    std::mutex m_lock;
};
