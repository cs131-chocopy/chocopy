cmake_minimum_required(VERSION 3.7.1)

project(ChocoPy-Compiler)

# required package flex bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(FMT REQUIRED)

# set the directory output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -g")
    # add_compile_options(-fsanitize=address)
    # add_link_options(-fsanitize=address)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

# Add block directories
add_subdirectory(libs)
add_subdirectory(src/cgen/stdlib)
add_subdirectory(src/parser)
add_subdirectory(src/semantic)
add_subdirectory(src/cgen)
add_subdirectory(src/ir-optimizer)
