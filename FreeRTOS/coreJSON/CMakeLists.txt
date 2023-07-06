# CMakeLists.txt for coreJSON library

cmake_minimum_required(VERSION 3.12)

# Set project name and programming language
project(coreJSON C CXX)

# Add the DMX library target
add_library(coreJSON
    source/core_json.c
)

# Include directories for your DMX library
target_include_directories(coreJSON
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/source/include
)