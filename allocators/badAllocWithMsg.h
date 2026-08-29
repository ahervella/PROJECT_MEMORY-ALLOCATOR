#pragma once
#include <cstddef>
#include <new>
#include <string>

class BadAllocWithMsg : public std::bad_alloc
{
public:
    explicit BadAllocWithMsg(std::string msg) : m_msg(std::move(msg)) {}
    const char* what() const noexcept override { return m_msg.c_str(); }
private:
    std::string m_msg;
};

template <class T>
inline void TestForConcreteClass(size_t size)
{
    if (size > sizeof(T))
    {
        // error, object or allocating space must be a subclass of T
        throw BadAllocWithMsg(std::string("Error, size of object does not match size of generic argument: ").append( typeid(T).name()));
    }
}