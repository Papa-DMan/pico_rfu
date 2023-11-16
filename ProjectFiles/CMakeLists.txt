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

cmake_minimum_required(VERSION 3.12)
add_executable(Pico_RFU_Full
    main.cpp
)

target_include_directories(Pico_RFU_Full PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dhcpserver
    ${CMAKE_CURRENT_SOURCE_DIR}/dnsserver
)

add_custom_command(
    OUTPUT bootloader.obj
    COMMAND ${PICO_TOOLCHAIN_PATH}/bin/arm-none-eabi-objcopy --input-target=binary --output-target=elf32-littlearm --binary-architecture=arm --rename-section .data=.ota_bootloader ${CMAKE_CURRENT_BINARY_DIR}/../bootloader/Pico_OTA_Bootloader.bin ${CMAKE_CURRENT_BINARY_DIR}/bootloader.obj
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(include_bootloader
    DEPENDS bootloader.obj
    DEPENDS Pico_OTA_Bootloader
)
add_dependencies(Pico_RFU_Full
    include_bootloader
)

target_link_libraries(Pico_RFU_Full 
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
    ${CMAKE_CURRENT_BINARY_DIR}/bootloader.obj
)

set_target_properties(Pico_RFU_Full PROPERTIES PICO_TARGET_LINKER_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/memmap_full_elf.ld)
# build dhcpserver/dhcpserver.c as a component of the target
target_sources(Pico_RFU_Full PRIVATE 
    dhcpserver/dhcpserver.c
    dnsserver/dnsserver.c
)



# disable usb output, enable uart output

pico_enable_stdio_uart(Pico_RFU_Full 1)
pico_enable_stdio_usb(Pico_RFU_Full 0)

pico_add_extra_outputs(Pico_RFU_Full)