#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/structs/rosc.h"

#include "pico/cyw43_arch.h"

#include "tcpserver.h"
#include "dhcpserver.h"
#include "dnsserver.h"

#define FIRMWARE_END 0x10200000

#define WIFI_FIRMWARE_START 0x10020000
#define WIFI_FIRMWARE_END 0x10056FD8

bool validate_firmware(uint32_t firmware_start, uint32_t firmware_end) {
    return (firmware_start - firmware_end > 0);

}

void ota_app_init(TCP_SERVER_T* state, dhcp_server_t* dhcp_server, dns_server_t* dns_server) {
    if (cyw43_arch_init()) {
        DEBUG_printf("Failed to initialize CYW4343\n");
        return;
    }

    state->context = cyw43_arch_async_context();
    const char *ap_name = "RFU_OTA";
    const char *password = "";

    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    dhcp_server_init(dhcp_server, &state->gw, &mask);

    dns_server_init(dns_server, &state->gw);

    if (!tcp_server_open(state, ap_name)) {
        DEBUG_printf("failed to open server\n");
        return;
    }

    state->complete = false;
}

void ota_app_work(TCP_SERVER_T* state) {
    while (!state->complete) {
        //tcp_server_poll(state);           //Only use this if using the pulling architecture
        sleep_ms(100);
    }
}

void ota_app_deinit(TCP_SERVER_T* state, dns_server_t* dns_server, dhcp_server_t* dhcp_server) {
    tcp_server_close(state);
    dns_server_deinit(dns_server);
    dhcp_server_deinit(dhcp_server);
    cyw43_arch_deinit();
    free(state);
}


int main() {
    stdio_usb_init();
    // Validate firmware
    uint32_t firmware_start = *(uint32_t*)0x10056FD8;
    uint32_t firmware_end = *(uint32_t*)0x10200000;
    if (firmware_start != 0xFFFFFFFF && firmware_end != 0xFFFFFFFF) {
        printf("Firmware start: %x\n", firmware_start);
        // Firmware is valid, jump to it
        bool valid = validate_firmware(firmware_start, firmware_end);
        if (valid) {
            printf("Firmware is valid\n");
            //disable interrupts
            asm volatile("cpsid i");
            //disable cache
            asm volatile("ldr r0, =0x0C000000");
            asm volatile("ldr r1, =0x1000");
            asm volatile("str r1, [r0]");

            //disable mmu
            asm volatile("ldr r0, =0x0C000000");
            asm volatile("mov r1, #0x1");
            asm volatile("str r1, [r0]");

            //disable branch prediction
            asm volatile("ldr r0, =0x0C000000");
            asm volatile("ldr r1, =0x8000");
            asm volatile("str r1, [r0, #4]");

            //flush data cache
            asm volatile("mov r0, #0");
            asm volatile("ldr r1, =0x0E0000F0");
            asm volatile("str r0, [r1]");

            //flush instruction cache
            asm volatile("mov r0, #0");
            asm volatile("ldr r1, =0x0E0000F8");
            asm volatile("str r0, [r1]");

            //flush branch target cache
            asm volatile("mov r0, #0");
            asm volatile("ldr r1, =0x0E0000F4");
            asm volatile("str r0, [r1]");

            //flush prefetch buffer
            asm volatile("mov r0, #0");
            asm volatile("ldr r1, =0x0E0000FC");
            asm volatile("str r0, [r1]");

            //flush TLB
            asm volatile("mov r0, #0");
            asm volatile("ldr r1, =0xE000ED9C");
            asm volatile("str r0, [r1]");

            // set VTOR [31:8] to 
            asm volatile("ldr r0, =0x10056FD8 + 0x100");
            asm volatile("mov r1, #8");
            asm volatile("lsr r0, r0, r1");
            asm volatile("ldr r1, =0xE000ED08");
            asm volatile("str r0, [r1]");

            // set stack pointer
            asm volatile("ldr r0, [r0, #4]");
            asm volatile("mov sp, r0");
            // jump to firmware
            asm volatile("bx r0");
        }
        else {
            // Firmware is invalid, start OTA app
            printf("Firmware is invalid\n");
            TCP_SERVER_T *state = (TCP_SERVER_T *)malloc(sizeof(TCP_SERVER_T));
            memset(state, 0, sizeof(TCP_SERVER_T));
            if (!state) {
                DEBUG_printf("Failed to allocate TCP server state\n");
                return 1;
            }
            dhcp_server_t dhcp_server;
            dns_server_t dns_server;
            ota_app_init(state, &dhcp_server, &dns_server);
            while (true) {
                ota_app_work(state);
            }
            ota_app_deinit(state, &dns_server, &dhcp_server);
            return 0;
        }
    }

}