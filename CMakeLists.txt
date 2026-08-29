cmake_minimum_required(VERSION 3.24)
project(MemoryAllocator)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/AllocatorTests.cpp)

find_package(benchmark REQUIRED)
find_package(GTest REQUIRED)

add_executable(AllocatorBenchmarks ${SOURCES})
target_compile_options(AllocatorBenchmarks PRIVATE -Wall -Wextra -Werror -O2 )
target_link_libraries(AllocatorBenchmarks PRIVATE benchmark::benchmark)

enable_testing()
add_executable(AllocatorTests AllocatorTests.cpp)
target_compile_options(AllocatorTests PRIVATE -Wall -Wextra -Werror -g -fsanitize=address -fno-omit-frame-pointer)
target_link_libraries(AllocatorTests PRIVATE GTest::gtest_main)
target_link_options(AllocatorTests PRIVATE -fsanitize=address)

include(GoogleTest)
gtest_discover_tests(AllocatorTests)
