project(Pico-OTA-Bootloader C CXX ASM)

add_executable(Pico_OTA_Bootloader
    main.cpp
)

target_include_directories(Pico_OTA_Bootloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    #${CMAKE_CURRENT_SOURCE_DIR}/dhcpserver
    #${CMAKE_CURRENT_SOURCE_DIR}/dnsserver
)

add_custom_command(
    OUTPUT firmware_data.obj
    COMMAND ${PICO_TOOLCHAIN_PATH}/bin/arm-none-eabi-objcopy --input-target=binary --output-target=elf32-littlearm --binary-architecture=arm --rename-section .data=.firmware ${CMAKE_CURRENT_SOURCE_DIR}/ota_wifi_firmware.bin ${CMAKE_CURRENT_BINARY_DIR}/firmware_data.obj
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(generate_firmware
    DEPENDS firmware_data.obj
)

target_link_libraries(Pico_OTA_Bootloader 
    pico_stdlib 
    hardware_flash
    ${CMAKE_CURRENT_BINARY_DIR}/firmware_data.obj
    #pico_cyw43_arch_lwip_poll
    #pico_lwip_http pico_lwip_mdns 
)


set_target_properties(Pico_OTA_Bootloader PROPERTIES PICO_TARGET_LINKER_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/memmap_boot.ld)

# build dhcpserver/dhcpserver.c as a component of the target
target_sources(Pico_OTA_Bootloader PRIVATE 
    #dhcpserver/dhcpserver.c
    #dnsserver/dnsserver.c
)

add_dependencies(Pico_OTA_Bootloader
    generate_firmware
)

# disable usb output, enable uart output

pico_enable_stdio_uart(Pico_OTA_Bootloader 0)
pico_enable_stdio_usb(Pico_OTA_Bootloader 1)

pico_add_extra_outputs(Pico_OTA_Bootloader)