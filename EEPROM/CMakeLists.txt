# CMakeLists.txt for EEPROM library

cmake_minimum_required(VERSION 3.12)

# Set project name and programming language
project(EEPROM C CXX)

# Add the EEPROM library target
add_library(EEPROM
    src/EEPROM.cpp
)

add_dependencies(EEPROM
    pico_stdlib
    hardware_flash
    hardware_sync
)

target_link_libraries(EEPROM
    pico_stdlib
    hardware_flash
    hardware_sync
)

# Include directories for EEPROM library
target_include_directories(EEPROM
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)