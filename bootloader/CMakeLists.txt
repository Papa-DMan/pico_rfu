cmake_minimum_required(VERSION 3.12)
add_executable(Pico_OTA_Bootloader
    main.cpp
)

target_include_directories(Pico_OTA_Bootloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dhcpserver
    ${CMAKE_CURRENT_SOURCE_DIR}/dnsserver
)

target_link_libraries(Pico_OTA_Bootloader 
    pico_stdlib 
    pico_cyw43_arch_lwip
    pico_lwip_http pico_lwip_mdns 
)
# build dhcpserver/dhcpserver.c as a component of the target
target_sources(Pico_OTA_Bootloader PRIVATE 
    dhcpserver/dhcpserver.c
    dnsserver/dnsserver.c
)

# disable usb output, enable uart output

pico_enable_stdio_uart(Pico_OTA_Bootloader 1)
pico_enable_stdio_usb(Pico_OTA_Bootloader 0)

pico_add_extra_outputs(Pico_OTA_Bootloader)