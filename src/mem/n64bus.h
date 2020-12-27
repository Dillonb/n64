#ifndef N64_N64BUS_H
#define N64_N64BUS_H

#include <util.h>
#include <system/n64system.h>
#include "addresses.h"

bool tlb_probe(dword vaddr, word* paddr, int* entry_number, cp0_t* cp0);

INLINE word resolve_virtual_address_32bit(word address, cp0_t* cp0) {
    word physical;
    switch (address >> 29) {
        // KSEG0
        case 0x4:
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            physical = address - SVREGION_KSEG0;
            logtrace("KSEG0: Translated 0x%08X to 0x%08X", address, physical);
            break;
        // KSEG1
        case 0x5:
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            physical = address - SVREGION_KSEG1;
            logtrace("KSEG1: Translated 0x%08X to 0x%08X", address, physical);
            break;
        // KUSEG
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3: {
            if (!tlb_probe(address, &physical, NULL, cp0)) {
                logfatal("Unimplemented: page miss translating virtual address 0x%08X in VREGION_KUSEG", address);
            }
            break;
        }
        // KSSEG
        case 0x6:
            logfatal("Unimplemented: translating virtual address 0x%08X in VREGION_KSSEG", address);
        // KSEG3
        case 0x7:
            if (!tlb_probe(address, &physical, NULL, cp0)) {
                logfatal("Unimplemented: page miss translating virtual address 0x%08X in VREGION_KSEG3", address);
            }
            break;
        default:
            logfatal("PANIC! should never end up here.");
    }
    return physical;
}

INLINE word resolve_virtual_address_64bit(dword address, cp0_t* cp0) {
    logfatal("Resolving virtual address 0x%016lX in 64 bit mode", address);
}

INLINE word resolve_virtual_address(dword address, cp0_t* cp0) {
    if (cp0->is_64bit_addressing) {
        return resolve_virtual_address_64bit(address, cp0);
    } else {
        return resolve_virtual_address_32bit(address, cp0);
    }
}


void n64_write_dword(n64_system_t* system, word address, dword value);
dword n64_read_dword(n64_system_t* system, word address);

void n64_write_word(n64_system_t* system, word address, word value);
word n64_read_word(word address);
word n64_read_physical_word(word address);

void n64_write_half(n64_system_t* system, word address, half value);
half n64_read_half(n64_system_t* system, word address);

void n64_write_byte(n64_system_t* system, word address, byte value);
byte n64_read_byte(n64_system_t* system, word address);

#endif //N64_N64BUS_H
