#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "hardware/structs/rosc.h"
#include "hardware/flash.h"
#include "pico/cyw43_arch.h"

// Bootloader size is 128KB
// Bootloader is 0x10000000 - 0x10002FFFF
#define BOOTLOADER_SIZE 0x00020000
#define FIRMWARE_START 0x10020000
//flash is 2MB
#define FIRMWARE_END 0x10200000

#define WIFI_FIRMWARE_START 0x10200000
#define WIFI_FIRMWARE_END 0x10240000

bool validate_firmware(uint32_t firmware_start, uint32_t firmware_end) {
    return true;
}


int main() {
    // Validate firmware
    uint32_t firmware_start = *(uint32_t*)FIRMWARE_START;
    uint32_t firmware_end = *(uint32_t*)FIRMWARE_END;
    if (firmware_start != 0xFFFFFFFF && firmware_end != 0xFFFFFFFF) {
        // Firmware is valid, jump to it
        bool valid = validate_firmware(firmware_start, firmware_end);
        if (valid) {
            //disable interrupts
            asm volatile("cpsid i");

            //disable cache
            asm volatile("ldr r0, =0x0C000000");
            asm volatile("mov r1, #0x1000");
            asm volatile("str r1, [r0]");

            //disable mmu
            asm volatile("ldr r0, =0x0C000000");
            asm volatile("mov r1, #0x1");
            asm volatile("str r1, [r0]");

            //disable branch prediction
            asm volatile("ldr r0, =0x0C000000");
            asm volatile("mov r1, #0x8000");
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

            // set VTOR to 
            asm volatile("ldr r0, =FIRMWARE_START + 0x100");
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
            cyw43_arch_init();
        }
    }

}