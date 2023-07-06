add_executable(blink
    main.cpp
)



target_include_directories(blink PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dhcpserver
)

target_link_libraries(blink pico_stdlib pico_cyw43_arch_lwip_sys_freertos pico_lwip_http pico_lwip_mdns FreeRTOS-Kernel FreeRTOS-Kernel-Heap4 DMX coreJSON)
# build dhcpserver/dhcpserver.c as a component of the target
target_sources(blink PRIVATE dhcpserver/dhcpserver.c)

# disable usb output, enable uart output

pico_enable_stdio_uart(blink 1)
pico_enable_stdio_usb(blink 0)

pico_add_extra_outputs(blink)