cmake_minimum_required(VERSION 3.5)
project(thread_pool)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -Wall")

add_executable(thread_pool test.cpp)

target_link_libraries(thread_pool -lpthread)