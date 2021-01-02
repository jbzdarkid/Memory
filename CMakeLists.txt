cmake_minimum_required(VERSION 3.6)

project(Memory)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(memory)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    # Sample code
    add_executable(sample_target target.cpp)
    add_executable(sample_attacker attacker.cpp)
    target_link_libraries(sample_attacker memory)
    install(TARGETS sample_target sample_attacker)
endif()
