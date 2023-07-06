# CMakeLists.txt for DMX library

cmake_minimum_required(VERSION 3.12)

# Set project name and programming language
project(DMX C CXX)

# Add the DMX library target
add_library(DMX
    src/piodmx.cpp
)

add_subdirectory(external/Pico-DMX)

# Include directories for your DMX library
target_include_directories(DMX
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link the Pico-DMX library
target_link_libraries(DMX
    PUBLIC
        Pico-DMX
)