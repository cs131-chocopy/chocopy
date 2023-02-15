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

# Add block directories
add_subdirectory(libs)
add_subdirectory(src/parser)
