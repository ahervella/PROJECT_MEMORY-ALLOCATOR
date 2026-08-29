#pragma once
#include "basicPoolMM.h"
#include <mutex>

template <class T>
class BasicPoolTSMM : public BasicPoolMM<T>
{
public:
    BasicPoolTSMM<T>(){};
    ~BasicPoolTSMM<T>() {};
    
    void* allocate(size_t size) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BasicPoolMM<T>::allocate(size);
    }

    void* allocateArray(size_t size) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        return BasicPoolMM<T>::allocateArray(size);
    }

    void free(void* p) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BasicPoolMM<T>::free(p);
    }

    void freeArray(void* p) override
    {
        const std::lock_guard<std::mutex> lock(m_lock);
        BasicPoolMM<T>::freeArray(p);
    }

private:
    std::mutex m_lock;
};
