#pragma once
#include "basicPoolAllocator.h"
#include <mutex>

template <class T>
class BasicPoolTSAllocator : public BasicPoolAllocator<T>
{
public:
    BasicPoolTSAllocator<T>(){};
    ~BasicPoolTSAllocator<T>() {};
    
    void* allocate(size_t size) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BasicPoolAllocator<T>::allocate(size);
    }

    void* allocateArray(size_t size) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BasicPoolAllocator<T>::allocateArray(size);
    }

    void free(void* p) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BasicPoolAllocator<T>::free(p);
    }

    void freeArray(void* p) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BasicPoolAllocator<T>::freeArray(p);
    }

private:
    std::mutex m_lock;
};
