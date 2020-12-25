cmake_minimum_required(VERSION 3.10)

project(Memory)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(memory)

add_executable(sample)
target_sources(sample PRIVATE main.cpp)
target_link_libraries(sample memory)
