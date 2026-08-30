
#include <chrono>
#include <ctime>
#include <iostream>

#include <benchmark/benchmark.h>

#include "allocators/BasicPoolAllocator.h"
#include "allocators/CustomMemoryAllocator.h"
#include "allocators/BitMapAllocator.h"
#include "Complex.h"

constexpr int LOOP_COUNT = 5000;
constexpr int ARRAY_SIZE = 1000;

const std::string DEFAULT = "Default";
const std::string BASIC_POOL = "Basic Pool";
const std::string BASIC_POOL_TS = "Basic Pool (Thread Safe)";
const std::string BIT_MAP = "Bit Map";
const std::string BIT_MAP_TS = "Bit Map (Thread Safe)";

void printResults(
    const std::string &name,
    std::chrono::time_point<std::chrono::high_resolution_clock> start,
    std::chrono::time_point<std::chrono::high_resolution_clock> stop )
{
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(stop - start);


    std::cout << std::endl
              << "--" + name + "--" << std::endl
              << " Performance time for " << LOOP_COUNT << " loops of " << ARRAY_SIZE << " new/delete ops: " << std::endl
              << duration.count() << " microseconds (or " << duration.count() / 1000000.0 << " seconds)" << std::endl
              << std::endl; 
}

template <class T>
auto controlBenchMarkSingleLoop( const int i )
{
    T* array[ARRAY_SIZE];
    for (int j = 0; j < ARRAY_SIZE; j++)
    {
        benchmark::DoNotOptimize(array[j] = new T(i, j));
    }

    for (int j = 0; j < ARRAY_SIZE; j++)
    {
        delete array[j];
    }
}

template<class T>
auto controlBenchMark(const std::string &name )
{
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LOOP_COUNT; i++)
    {
        controlBenchMarkSingleLoop<T>(i);
    }

    auto stopTime = std::chrono::high_resolution_clock::now();
    printResults( name, startTime, stopTime);
}

template <class AllocatorT, class T>
void benchMarkSingleLoop( const int i, AllocatorT& allocator )
{
    T* array[ARRAY_SIZE];
    for (int j = 0; j < ARRAY_SIZE; j++)
    {
        benchmark::DoNotOptimize(array[j] = new (allocator) T(i, j));
    }

    for (int j = 0; j < ARRAY_SIZE; j++)
    {
        operator delete<T>( array[j], allocator);
    }
}

template<class AllocatorT, class T>
auto benchMark( const std::string &name )
{
    AllocatorT allocator;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LOOP_COUNT; i++)
    {
        benchMarkSingleLoop<AllocatorT, T>(i, allocator);
    }

    auto stopTime = std::chrono::high_resolution_clock::now();

    printResults( name, startTime, stopTime);
}

template<class T>
static void BM_ControlAllocate(benchmark::State &state)
{
    for (auto _ : state)
    {
        controlBenchMarkSingleLoop<T>(9);
    }
}

template<class AllocatorT, class T>
static void BM_Allocate(benchmark::State &state, AllocatorT& allocator)
{
    for (auto _ : state)
    {
        benchMarkSingleLoop<AllocatorT, T>(9, allocator);
    }
}


BasicPoolAllocator<Complex> basicPool;
BasicPoolTSAllocator<Complex> basicPoolTS;
BitMapAllocator<Complex> bitMap;
BitMapTSAllocator<Complex> bitMapTS;

BENCHMARK(BM_ControlAllocate<Complex>)->Name(DEFAULT);
BENCHMARK_TEMPLATE2_CAPTURE(BM_Allocate, BasicPoolAllocator<Complex>, Complex, BASIC_POOL, basicPool);
BENCHMARK_TEMPLATE2_CAPTURE(BM_Allocate, BasicPoolTSAllocator<Complex>, Complex, BASIC_POOL_TS, basicPoolTS);
BENCHMARK_TEMPLATE2_CAPTURE(BM_Allocate, BitMapAllocator<Complex>, Complex, BIT_MAP, bitMap);
BENCHMARK_TEMPLATE2_CAPTURE(BM_Allocate, BitMapTSAllocator<Complex>, Complex, BIT_MAP_TS, bitMapTS);

int main(int argc, char* argv[])
{
    controlBenchMark<Complex>(DEFAULT);
    benchMark<BasicPoolAllocator<Complex>, Complex>(BASIC_POOL);
    benchMark<BasicPoolTSAllocator<Complex>, Complex>(BASIC_POOL_TS);
    benchMark<BitMapAllocator<Complex>, Complex>(BIT_MAP);
    benchMark<BitMapTSAllocator<Complex>, Complex>(BIT_MAP_TS);

   ::benchmark::Initialize(&argc, argv);
   ::benchmark::RunSpecifiedBenchmarks();
   ::benchmark::Shutdown();

   std::cout<<std::endl;
}
