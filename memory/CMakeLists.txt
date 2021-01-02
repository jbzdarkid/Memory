add_library(memory)

target_include_directories(memory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(memory PRIVATE 
    HazardPointer.cpp
    HazardPointer.h
    Memory.cpp
    Memory.h
    ThreadSafeAddressMap.cpp
    ThreadSafeAddressMap.h
    Utils.cpp
    Utils.h
)

add_executable(snoopy)
target_sources(snoopy PRIVATE snoopy.cpp)

if(WIN32)
    message("Building on Windows")
    target_compile_definitions(memory PRIVATE MEMORY_PLATFORM_WINDOWS)
    target_sources(memory PRIVATE Memory_Win.h Memory_Win.cpp)
elseif(UNIX AND NOT APPLE)
    message("Building on Linux")
    target_compile_definitions(memory PRIVATE MEMORY_PLATFORM_LINUX)
    target_sources(memory PRIVATE snoopy.cpp Memory_Linux.h Memory_Linux.cpp)

    target_compile_definitions(memory PRIVATE SNOOPY_MAIN=1)
elseif(UNIX AND APPLE)
    message("Building on Mac")
    target_compile_definitions(memory PRIVATE MEMORY_PLATFORM_MAC)
    target_sources(memory PRIVATE Memory_Mac.h Memory_Mac.cpp)
endif()
