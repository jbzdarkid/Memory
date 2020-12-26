add_library(memory)

target_include_directories(memory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB sources CONFIGURE_DEPENDS "*.cpp" "*.h")
target_sources(memory PRIVATE ${sources} ${headers})
