#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "hardware/structs/rosc.h"
#include "hardware/structs/flash.h"
#include "pico/cyw43_arch.h"

// Bootloader size is 16KB
// Bootloader is 0x00000000 - 0x00003FFF
#define BOOTLOADER_SIZE 0x4000
#define FIRMWARE_START 0x00004000
// adjust this to the entry of your firmware and make sure to build firmware with this entry point address
#define FIRMWARE_ENTRY 0x00004000 
//flash is 2MB
#define FIRMWARE_END 0x200000

bool validate_firmware(uint32_t* firmware_start, uint32_t* firmware_end) {
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
            asm volatile("mrs r0, SCTLR");
            asm volatile("bic r0, r0, #0x4");
            asm volatile("msr SCTLR, r0");
            //disable mmu
            asm volatile("mrs r0, SCTLR");
            asm volatile("bic r0, r0, #0x1");
            asm volatile("msr SCTLR, r0");
            //disable branch prediction
            asm volatile("mrs r0, SCTLR");
            asm volatile("bic r0, r0, #0x1000");
            asm volatile("msr SCTLR, r0");
            //flush data cache
            asm volatile("mov r0, #0");
            asm volatile("mcr p15, 0, r0, c7, c10, 0");
            //flush instruction cache
            asm volatile("mov r0, #0");
            asm volatile("mcr p15, 0, r0, c7, c5, 0");
            //flush branch target cache
            asm volatile("mov r0, #0");
            asm volatile("mcr p15, 0, r0, c7, c5, 6");
            //flush prefetch buffer
            asm volatile("mov r0, #0");
            asm volatile("mcr p15, 0, r0, c7, c5, 4");
            //flush TLB
            asm volatile("mov r0, #0");
            asm volatile("mcr p15, 0, r0, c8, c7, 0");

            // set VTOR to 
            asm volatile("ldr r0, =FIRMWARE_START + 0x100");
            // right shift by 8
            
            asm volatile("ldr r0, =0xE000ED08");

            // set stack pointer
            asm volatile("ldr r1, [r0]");
            asm volatile("mov sp, r1");

            // load to r1, r0 + 4
            asm volatile("inc r0 #4");
            asm volatile("bx r0");
        }
        else {
            // Firmware is invalid, start OTA app
            cyw43_arch_init();
        }
    }

}