#ifndef N64_N64BUS_H
#define N64_N64BUS_H

#include <util.h>
#include <system/n64system.h>
#include "addresses.h"

bool tlb_probe(word vaddr, word* paddr, int* entry_number, cp0_t* cp0);

INLINE word resolve_virtual_address(word address, cp0_t* cp0) {
    word physical;
    switch (address) {
        case VREGION_KUSEG: {
            if (tlb_probe(address, &physical, NULL, cp0)) {
                //printf("TLB translation 0x%08x -> 0x%08x\n", address, physical);
            } else {
                logfatal("Unimplemented: page miss translating virtual address 0x%08X in VREGION_KUSEG", address);
            }
            break;
        }
        case VREGION_KSEG0:
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            physical = address - SVREGION_KSEG0;
            logtrace("KSEG0: Translated 0x%08X to 0x%08X", address, physical);
            break;
        case VREGION_KSEG1:
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            physical = address - SVREGION_KSEG1;
            logtrace("KSEG1: Translated 0x%08X to 0x%08X", address, physical);
            break;
        case VREGION_KSSEG:
            logfatal("Unimplemented: translating virtual address in VREGION_KSSEG");
        case VREGION_KSEG3:
            logfatal("Unimplemented: translating virtual address in VREGION_KSEG3");
        default:
            logfatal("Address 0x%08X doesn't really look like a virtual address", address);
    }
    return physical;
}


void n64_write_dword(n64_system_t* system, word address, dword value);
dword n64_read_dword(n64_system_t* system, word address);

void n64_write_word(n64_system_t* system, word address, word value);
word n64_read_word(n64_system_t* system, word address);

void n64_write_half(n64_system_t* system, word address, half value);
half n64_read_half(n64_system_t* system, word address);

void n64_write_byte(n64_system_t* system, word address, byte value);
byte n64_read_byte(n64_system_t* system, word address);

#endif //N64_N64BUS_H
