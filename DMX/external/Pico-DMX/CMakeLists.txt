# CMakeLists.txt for Pico-DMX library

# Set library to CXX (C++), as the Pico-DMX library is written in C++
project(Pico-DMX C CXX)


# Add the Pico-DMX library target
add_library(Pico-DMX
    # List your Pico-DMX library source files here
    src/DmxInput.cpp
    src/DmxOutput.cpp
)

# Include directories for Pico-DMX
target_include_directories(Pico-DMX
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add any other configurations specific to the Pico-DMX library
# ...
target_link_libraries(Pico-DMX
    pico_stdlib
    hardware_pio
    hardware_dma
)

