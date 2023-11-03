cmake_minimum_required(VERSION 3.14)
project(mudong-rpc CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(CXX_FLAGS
        -fno-omit-frame-pointer # linux perf
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wno-unused-parameter
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        -Wshadow
        -Wwrite-strings
        -std=c++20
        -march=native
        -rdynamic)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

message("PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(
        third_party
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_BINARY_DIR})

add_subdirectory(third_party/mudong-json)
add_subdirectory(third_party/mudong-ev)
add_subdirectory(src)

if (CMAKE_BUILD_EXAMPLES)
        add_subdirectory(examples)
endif()