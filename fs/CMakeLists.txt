cmake_minimum_required(VERSION 3.12)
project(makefscustomdata)

# Find the gzip command
find_program(GZIP gzip)

# Define the website directory
set(WEBSITE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/website)

# Define the output directory for the compressed files
set(COMPRESSED_DIR ${CMAKE_CURRENT_BINARY_DIR}/compressed)
make_directory(${COMPRESSED_DIR})

# Define the output directory for the fsdata_custom.c file
set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Define the list of website files to compress
set(WEBSITE_FILES
    ${WEBSITE_DIR}/favicon.ico
    ${WEBSITE_DIR}/index.html
    ${WEBSITE_DIR}/keypad.html
    ${WEBSITE_DIR}/keypad.js
    ${WEBSITE_DIR}/login.js
    ${WEBSITE_DIR}/settings.html
    ${WEBSITE_DIR}/settings.js
    ${WEBSITE_DIR}/styles.css
    
)

# Define the list of compressed website files
set(COMPRESSED_FILES "")
foreach(FILE ${WEBSITE_FILES})
    get_filename_component(FILENAME ${FILE} NAME)
    set(COMPRESSED_FILE ${COMPRESSED_DIR}/${FILENAME}.gz)
    list(APPEND COMPRESSED_FILES ${COMPRESSED_FILE})
    add_custom_command(
        OUTPUT ${COMPRESSED_FILE}
        COMMAND ${GZIP} -9 -c ${FILE} > ${COMPRESSED_FILE}
        DEPENDS ${FILE}
        COMMENT "Compressing ${FILE}"
    )
endforeach()

# Define the custom command to generate the fsdata_custom.c file
add_custom_command(
    OUTPUT ${OUTPUT_DIR}/fsdata_custom.c
    COMMAND ${CMAKE_COMMAND} -E chdir ${WEBSITE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/makefsdata && ${CMAKE_COMMAND} -E copy ${WEBSITE_DIR}/fsdata_custom.c ${OUTPUT_DIR}/fsdata_custom.c && ${CMAKE_COMMAND} -E remove ${WEBSITE_DIR}/fsdata_custom.c
    DEPENDS ${COMPRESSED_FILES}
    COMMENT "Generating fsdata_custom.c"
)

# Define the custom target to build the fsdata_custom.c file
add_custom_target(fsdata_custom DEPENDS ${OUTPUT_DIR}/fsdata_custom.c)
