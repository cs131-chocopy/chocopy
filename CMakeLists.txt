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

# Add LLVM toolchain support
set(CMAKE_C_FLAGS "-std=c99")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O4 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O4 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# Add block directories
add_subdirectory(libs)
add_subdirectory(src/cgen/stdlib)
add_subdirectory(src/parser)
add_subdirectory(src/semantic)
add_subdirectory(src/cgen)
add_subdirectory(src/ir-optimizer)
