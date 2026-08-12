#include <chrono>
#include <iostream>
#include "complex.h"
#include "globalManager.h"

auto benchMark(bool useCustomAllocator)
{
    ::useCustomAllocator = useCustomAllocator;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 5000; i++)
    {
        Complex* array[1000];
        for (int j = 0; j < 1000; j++)
        {
            array[j] = new Complex(i, j);
        }

        for (int j = 0; j < 1000; j++)
        {
            delete array[j];
        }
    }

    auto stopTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        stopTime - startTime);

    std::cout
        << std::endl
        << std::endl
        << (useCustomAllocator ? "--Custom Allocator--" : "--Default Allocator--")
        << std::endl
        <<" Performance time: "
        << duration.count()
        << " microseconds (or "
        << duration.count() / 1000000.0
        << " seconds)"
        << std::endl
        << std::endl;
}

int main()//int argc, char* argv[])
{
    benchMark(true);
    benchMark(false);

    return 0;
}