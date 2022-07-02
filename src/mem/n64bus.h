#ifndef N64_N64BUS_H
#define N64_N64BUS_H

#include <util.h>
#include <system/n64system.h>
#include "addresses.h"

tlb_entry_t* find_tlb_entry(dword vaddr, int* entry_number);
bool tlb_probe(dword vaddr, bus_access_t bus_access, word* paddr, int* entry_number);

#define REGION_XKUSEG 0x0000000000000000 ... 0x000000FFFFFFFFFF
#define REGION_XBAD1  0x0000010000000000 ... 0x3FFFFFFFFFFFFFFF
#define REGION_XKSSEG 0x4000000000000000 ... 0x400000FFFFFFFFFF
#define REGION_XBAD2  0x4000010000000000 ... 0x7FFFFFFFFFFFFFFF
#define REGION_XKPHYS 0x8000000000000000 ... 0xBFFFFFFFFFFFFFFF
#define REGION_XKSEG  0xC000000000000000 ... 0xC00000FF7FFFFFFF
#define REGION_XBAD3  0xC00000FF80000000 ... 0xFFFFFFFF7FFFFFFF
#define REGION_CKSEG0 0xFFFFFFFF80000000 ... 0xFFFFFFFF9FFFFFFF
#define REGION_CKSEG1 0xFFFFFFFFA0000000 ... 0xFFFFFFFFBFFFFFFF
#define REGION_CKSSEG 0xFFFFFFFFC0000000 ... 0xFFFFFFFFDFFFFFFF
#define REGION_CKSEG3 0xFFFFFFFFE0000000 ... 0xFFFFFFFFFFFFFFFF

INLINE bool resolve_virtual_address_32bit(word address, bus_access_t bus_access, word* physical) {
    switch (address >> 29) {
        // KSEG0
        case 0x4:
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            *physical = address - SVREGION_KSEG0;
            logtrace("KSEG0: Translated 0x%08X to 0x%08X", address, *physical);
            break;
        // KSEG1
        case 0x5:
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            *physical = address - SVREGION_KSEG1;
            logtrace("KSEG1: Translated 0x%08X to 0x%08X", address, *physical);
            break;
        // KUSEG
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3: {
            return tlb_probe(se_32_64(address), bus_access, physical, NULL);
        }
        // KSSEG
        case 0x6:
            logfatal("Unimplemented: translating virtual address 0x%08X in VREGION_KSSEG", address);
        // KSEG3
        case 0x7:
            return tlb_probe(se_32_64(address), bus_access, physical, NULL);
        default:
            logfatal("PANIC! should never end up here.");
    }
    return true;
}

INLINE bool resolve_virtual_address_user_32bit(word address, bus_access_t bus_access, word* physical) {
    switch (address) {
        case VREGION_KUSEG:
            return tlb_probe(se_32_64(address), bus_access, physical, NULL);
        default:
            N64CP0.tlb_error = TLB_ERROR_DISALLOWED_ADDRESS;
            return false;
    }
}

INLINE bool resolve_virtual_address_64bit(dword address, bus_access_t bus_access, word* physical) {
    switch (address) {
        case REGION_XKUSEG:
            return tlb_probe(address, bus_access, physical, NULL);
        case REGION_XKSSEG:
            return tlb_probe(address, bus_access, physical, NULL);
        case REGION_XKPHYS: {
            if (!N64CP0.kernel_mode) {
                logfatal("Access to XKPHYS address 0x%016lX when outside kernel mode!", address);
            }
            byte high_two_bits = (address >> 62) & 0b11;
            if (high_two_bits != 0b10) {
                logfatal("Access to XKPHYS address 0x%016lX with high two bits != 0b10!", address);
            }
            byte subsegment = (address >> 59) & 0b11;
            bool cached = subsegment != 2;
            if (cached) {
                //logwarn("Resolving virtual address in cached XKPHYS subsegment %d", subsegment);
            }
            // If any bits in the range of 58:32 are set, the address is invalid.
            bool valid = (address & 0x07FFFFFF00000000) == 0;
            if (!valid) {
                N64CP0.tlb_error = TLB_ERROR_DISALLOWED_ADDRESS;
                return false;
            }
            *physical = address & 0xFFFFFFFF;
            break;
        }
        case REGION_XKSEG:
            return tlb_probe(address, bus_access, physical, NULL);
        case REGION_CKSEG0:
            // Identical to kseg0 in 32 bit mode.
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            *physical = address - SVREGION_KSEG0; // Implies cutting off the high 32 bits
            logtrace("CKSEG0: Translated 0x%016lX to 0x%08X", address, *physical);
            break;
        case REGION_CKSEG1:
            // Identical to kseg1 in 32 bit mode.
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            *physical = address - SVREGION_KSEG1; // Implies cutting off the high 32 bits
            logtrace("KSEG1: Translated 0x%016lX to 0x%08X", address, *physical);
            break;
        case REGION_CKSSEG:
            logfatal("Resolving virtual address 0x%016lX (REGION_CKSSEG) in 64 bit mode", address);
        case REGION_CKSEG3:
            return tlb_probe(address, bus_access, physical, NULL);
        case REGION_XBAD1:
        case REGION_XBAD2:
        case REGION_XBAD3:
            N64CP0.tlb_error = TLB_ERROR_DISALLOWED_ADDRESS;
            return false;
        default:
            logfatal("Resolving virtual address 0x%016lX in 64 bit mode", address);
    }

    return true;
}

INLINE bool resolve_virtual_address_user_64bit(dword address, bus_access_t bus_access, word* physical) {
    switch (address) {
        case REGION_XKUSEG:
            return tlb_probe(address, bus_access, physical, NULL);
        default:
            N64CP0.tlb_error = TLB_ERROR_DISALLOWED_ADDRESS;
            return false;
    }
}

INLINE bool resolve_virtual_address(dword virtual, bus_access_t bus_access, word* physical) {
    if (unlikely(N64CP0.is_64bit_addressing)) {
        if (likely(N64CP0.kernel_mode)) {
            return resolve_virtual_address_64bit(virtual, bus_access, physical);
        } else if (N64CP0.user_mode) {
            return resolve_virtual_address_user_64bit(virtual, bus_access, physical);
        } else if (N64CP0.supervisor_mode) {
            logfatal("Supervisor mode memory access, 64 bit mode");
        } else {
            logfatal("Unknown mode! This should never happen!");
        }
    } else {
        if (likely(N64CP0.kernel_mode)) {
            return resolve_virtual_address_32bit(virtual, bus_access, physical);
        } else if (N64CP0.user_mode) {
            return resolve_virtual_address_user_32bit(virtual, bus_access, physical);
        } else if (N64CP0.supervisor_mode) {
            logfatal("Supervisor mode memory access, 32 bit mode");
        } else {
            logfatal("Unknown mode! This should never happen!");
        }
    }
}

INLINE word resolve_virtual_address_or_die(dword virtual, bus_access_t bus_access) {
    word physical;
    if (!resolve_virtual_address(virtual, bus_access, &physical)) {
        logfatal("Unhandled TLB exception at 0x%016lX! Stop calling resolve_virtual_address_or_die() here!", virtual);
    }
    return physical;
}

void n64_write_physical_dword(word address, dword value);
dword n64_read_physical_dword(word address);

void n64_write_physical_word(word address, word value);
word n64_read_physical_word(word address);

void n64_write_physical_half(word address, half value);
half n64_read_physical_half(word address);

void n64_write_physical_byte(word address, byte value);
byte n64_read_physical_byte(word address);

INLINE void n64_write_word(dword address, word value) {
    n64_write_physical_word(resolve_virtual_address_or_die(address, true), value);
}

INLINE word n64_read_word(dword address) {
    return n64_read_physical_word(resolve_virtual_address_or_die(address, false));
}
INLINE byte n64_read_byte(dword address) {
    return n64_read_physical_byte(resolve_virtual_address_or_die(address, false));
}

#endif //N64_N64BUS_H
