cmake_minimum_required(VERSION 3.12)
add_executable(Pico_RFU
    main.cpp
)

target_include_directories(Pico_RFU PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dhcpserver
    ${CMAKE_CURRENT_SOURCE_DIR}/dnsserver
)

target_link_libraries(Pico_RFU 
    pico_stdlib 
    pico_cyw43_arch_lwip_sys_freertos 
    pico_lwip_http pico_lwip_mdns 
    FreeRTOS-Kernel 
    FreeRTOS-Kernel-Heap4 
    DMX 
    coreJSON
    pico_lwip_mbedtls
    pico_mbedtls
    EEPROM
)

set_target_properties(Pico_RFU PROPERTIES PICO_TARGET_LINKER_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/memmap_ota.ld)
# build dhcpserver/dhcpserver.c as a component of the target
target_sources(Pico_RFU PRIVATE 
    dhcpserver/dhcpserver.c
    dnsserver/dnsserver.c
)

# disable usb output, enable uart output

pico_enable_stdio_uart(Pico_RFU 1)
pico_enable_stdio_usb(Pico_RFU 0)

pico_add_extra_outputs(Pico_RFU)