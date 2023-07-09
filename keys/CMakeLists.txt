cmake_minimum_required(VERSION 3.12)
project(KeysLibrary)

# Find Python
find_package(Python REQUIRED)

# Set the output directory
set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_headers")

# Create the output directory
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Get a list of all PEM files in the current directory
file(GLOB PEM_FILES "*.pem")

# Iterate over each PEM file and generate the corresponding header file
foreach(PEM_FILE ${PEM_FILES})
    get_filename_component(FILE_NAME ${PEM_FILE} NAME_WE)
    set(HEADER_FILE "${OUTPUT_DIR}/${FILE_NAME}.h")
    set(VARIABLE_NAME "${FILE_NAME}_data")

    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/script.py ${PEM_FILE} ${HEADER_FILE} ${VARIABLE_NAME}
        RESULT_VARIABLE SCRIPT_RESULT
    )
    if (SCRIPT_RESULT EQUAL 0)
        message("Generated header: ${HEADER_FILE}")
    else()
        message(FATAL_ERROR "Failed to generate header for file: ${PEM_FILE}")
    endif()
endforeach()

# Create the library target
add_library(keys_library INTERFACE)
target_include_directories(keys_library INTERFACE ${OUTPUT_DIR})

# Add any other target-specific configurations, dependencies, etc.
