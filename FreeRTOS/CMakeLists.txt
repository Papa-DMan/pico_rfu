# CMakeLists.txt for coreJSON library

cmake_minimum_required(VERSION 3.12)

# Set project name and programming language
project(coreJSON C CXX)

# Add the coreJSON library target
add_library(coreJSON
    ${CMAKE_CURRENT_SOURCE_DIR}/coreJSON/source/core_json.c
)

# Include directories for coreJSON library
target_include_directories(coreJSON
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/coreJSON/source/include
)