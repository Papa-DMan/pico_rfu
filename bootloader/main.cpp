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
            uint32_t* firmware_entry = (uint32_t*)FIRMWARE_START;
            asm volatile("mov pc, %0" : : "r" (firmware_entry));
        }
        else {
            // Firmware is invalid, start OTA app
            cyw43_arch_init();
        }
    }

}