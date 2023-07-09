cmake_minimum_required(VERSION 3.12)
project(encryption_keys)

# Find Python
find_package(Python REQUIRED COMPONENTS Interpreter)

# Define the Python script path
set(PYTHON_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pemtoc.py)

# Define the output directory for generated headers
make_directory(${CMAKE_BINARY_DIR}/generated_headers)
set(GENERATED_HEADERS_DIR ${CMAKE_BINARY_DIR}/generated_headers)

# Define the input file, destination file, and variable name
set(INPUT_FILE ${CMAKE_CURRENT_LIST_DIR}/private_unencrypted.pem)
set(DESTINATION_FILE ${GENERATED_HEADERS_DIR}/encryption_keys.h)

# Run the Python script to generate headers
execute_process(
    COMMAND ${Python_EXECUTABLE} ${PYTHON_SCRIPT} ${INPUT_FILE} ${DESTINATION_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RESULT_VARIABLE PYTHON_SCRIPT_RESULT
)

if(PYTHON_SCRIPT_RESULT EQUAL 0)
    message(STATUS "Header generation successful.")
else()
    message(FATAL_ERROR "Failed to generate headers.")
endif()

# Include the generated headers directory
include_directories(${GENERATED_HEADERS_DIR})

# Add your library source files here
add_library(encryption_keys INTERFACE)

# Add the generated headers to YourLibrary target
# target_sources(YourLibrary INTERFACE ${DESTINATION_FILE})
target_include_directories(encryption_keys INTERFACE ${GENERATED_HEADERS_DIR})

# Specify the installation rules if needed
install(DIRECTORY ${GENERATED_HEADERS_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
