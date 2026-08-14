cmake_minimum_required(VERSION 3.24)
project(MemoryAllocator)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES CONFIGURE_DEPENDS ./*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
#target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -fsanitize=address -g)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -g)
#target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
