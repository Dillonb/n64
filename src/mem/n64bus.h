#ifndef N64_N64BUS_H
#define N64_N64BUS_H

#include <util.h>
#include <system/n64system.h>
#include "addresses.h"

INLINE bool is_tlb(u64 vaddr) {
    switch (vaddr) {
        case VREGION_XKUSEG:
        case VREGION_XKSSEG:
        case VREGION_XKSEG:
        case VREGION_CKSEG3:
        case VREGION_CKSSEG:

        case VREGION_XBAD1:
        case VREGION_XBAD2:
        case VREGION_XBAD3:
            return true;

        case VREGION_XKPHYS:
        case VREGION_CKSEG0:
        case VREGION_CKSEG1:
            return false;
        default:
            logfatal("Should never get here! Address %016" PRIX64 " did not match any region.", vaddr);
    }
}

tlb_entry_t* find_tlb_entry(u64 vaddr, int* entry_number);
bool tlb_probe(u64 vaddr, bus_access_t bus_access, bool* cached, u32* paddr, int* entry_number);

INLINE bool resolve_virtual_address_32bit(u64 address, bus_access_t bus_access, bool* cached, u32* physical) {
    switch ((address >> 29) & 0x7) {
        // Direct mapping. Simply mask the virtual address to get the physical address
        case 0x4: // KSEG0
            *cached = true;
            *physical = address & 0x1FFFFFFF;
            break;
        case 0x5: // KSEG1
            *cached = false;
            *physical = address & 0x1FFFFFFF;
            break;

        // KUSEG
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
            return tlb_probe(se_32_64(address), bus_access, cached, physical, NULL);
        // KSSEG
        case 0x6:
            return tlb_probe(se_32_64(address), bus_access, cached, physical, NULL);
        // KSEG3
        case 0x7:
            return tlb_probe(se_32_64(address), bus_access, cached, physical, NULL);
    }
    return true;
}

INLINE bool resolve_virtual_address_user_32bit(u64 address, bus_access_t bus_access, bool* cached, u32* physical) {
    switch (address) {
        case VREGION_KUSEG:
            return tlb_probe(se_32_64(address), bus_access, cached, physical, NULL);
        default:
            N64CP0.tlb_error = TLB_ERROR_DISALLOWED_ADDRESS;
            return false;
    }
}

INLINE bool resolve_virtual_address_64bit(u64 address, bus_access_t bus_access, bool* cached, u32* physical) {
    switch (address) {
        case VREGION_XKUSEG:
            return tlb_probe(address, bus_access, cached, physical, NULL);
        case VREGION_XKSSEG:
            return tlb_probe(address, bus_access, cached, physical, NULL);
        case VREGION_XKPHYS: {
            if (!N64CP0.kernel_mode) {
                logfatal("Access to XKPHYS address 0x%016" PRIX64 " when outside kernel mode!", address);
            }
            u8 high_two_bits = (address >> 62) & 0b11;
            if (high_two_bits != 0b10) {
                logfatal("Access to XKPHYS address 0x%016" PRIX64 " with high two bits != 0b10!", address);
            }
            u8 subsegment = (address >> 59) & 0b11;
            *cached = subsegment != 2;
            // If any bits in the range of 58:32 are set, the address is invalid.
            bool valid = (address & 0x07FFFFFF00000000) == 0;
            if (!valid) {
                N64CP0.tlb_error = TLB_ERROR_DISALLOWED_ADDRESS;
                return false;
            }
            *physical = address & 0xFFFFFFFF;
            break;
        }
        case VREGION_XKSEG:
            return tlb_probe(address, bus_access, cached, physical, NULL);
        case VREGION_CKSEG0:
            // Identical to kseg0 in 32 bit mode.
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            *physical = address - SVREGION_KSEG0; // Implies cutting off the high 32 bits
            *cached = true;
            logtrace("CKSEG0: Translated 0x%016" PRIX64 " to 0x%08X", address, *physical);
            break;
        case VREGION_CKSEG1:
            // Identical to kseg1 in 32 bit mode.
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            *physical = address - SVREGION_KSEG1; // Implies cutting off the high 32 bits
            *cached = false;
            logtrace("KSEG1: Translated 0x%016" PRIX64 " to 0x%08X", address, *physical);
            break;
        case VREGION_CKSSEG:
            logfatal("Resolving virtual address 0x%016" PRIX64 " (VREGION_CKSSEG) in 64 bit mode", address);
        case VREGION_CKSEG3:
            return tlb_probe(address, bus_access, cached, physical, NULL);
        case VREGION_XBAD1:
        case VREGION_XBAD2:
        case VREGION_XBAD3:
            N64CP0.tlb_error = TLB_ERROR_DISALLOWED_ADDRESS;
            return false;
        default:
            logfatal("Resolving virtual address 0x%016" PRIX64 " in 64 bit mode", address);
    }

    return true;
}

INLINE bool resolve_virtual_address_user_64bit(u64 address, bus_access_t bus_access, bool* cached, u32* physical) {
    switch (address) {
        case VREGION_XKUSEG:
            return tlb_probe(address, bus_access, cached, physical, NULL);
        default:
            N64CP0.tlb_error = TLB_ERROR_DISALLOWED_ADDRESS;
            return false;
    }
}

INLINE resolve_virtual_address_handler get_resolve_virtual_address_handler() {
    if (unlikely(N64CP0.is_64bit_addressing)) {
        if (likely(N64CP0.kernel_mode)) {
            return resolve_virtual_address_64bit;
        } else if (N64CP0.user_mode) {
            return resolve_virtual_address_user_64bit;
        } else if (N64CP0.supervisor_mode) {
            logfatal("Supervisor mode memory access, 64 bit mode");
        } else {
            logfatal("Unknown mode! This should never happen!");
        }
    } else {
        if (likely(N64CP0.kernel_mode)) {
            return resolve_virtual_address_32bit;
        } else if (N64CP0.user_mode) {
            return resolve_virtual_address_user_32bit;
        } else if (N64CP0.supervisor_mode) {
            logfatal("Supervisor mode memory access, 32 bit mode");
        } else {
            logfatal("Unknown mode! This should never happen!");
        }
    }
}

#define resolve_virtual_address(vaddr, bus_access, cached, physical) N64CP0.resolve_virtual_address(vaddr, bus_access, cached, physical)

INLINE u32 resolve_virtual_address_or_die(u64 vaddr, bus_access_t bus_access, bool* cached) {
    u32 physical;
    if (!resolve_virtual_address(vaddr, bus_access, cached, &physical)) {
        logfatal("Unhandled TLB exception at 0x%016" PRIX64 "! Stop calling resolve_virtual_address_or_die() here!", vaddr);
    }
    return physical;
}

void n64_write_physical_dword(u32 address, u64 value);
u64 n64_read_physical_dword(u32 address);

void n64_write_physical_word(u32 address, u32 value);
u32 n64_read_physical_word(u32 address);

void n64_write_physical_half(u32 address, u32 value);
u16 n64_read_physical_half(u32 address);

void n64_write_physical_byte(u32 address, u32 value);
u8 n64_read_physical_byte(u32 address);

bool debugger_read_physical_byte(u32 address, u8* result);

INLINE u8 n64_read_byte(u64 address) {
    bool cached;
    return n64_read_physical_byte(resolve_virtual_address_or_die(address, BUS_LOAD, &cached));
}

#ifdef ENABLE_DCACHE
INLINE u8 conditional_cache_read_byte(bool cached, u64 vaddr, u32 paddr) {
    if (cached) {
        return cache_read_byte(vaddr, paddr);
    } else {
        return n64_read_physical_byte(paddr);
    }
}

INLINE void conditional_cache_write_byte(bool cached, u64 vaddr, u32 paddr, u8 value) {
    if (cached) {
        cache_write_byte(vaddr, paddr, value);
    } else {
        n64_write_physical_byte(paddr, value);
    }
}

INLINE u16 conditional_cache_read_half(bool cached, u64 vaddr, u32 paddr) {
    if (cached) {
        return cache_read_half(vaddr, paddr);
    } else {
        return n64_read_physical_half(paddr);
    }
}

INLINE void conditional_cache_write_half(bool cached, u64 vaddr, u32 paddr, u16 value) {
    if (cached) {
        cache_write_half(vaddr, paddr, value);
    } else {
        n64_write_physical_half(paddr, value);
    }
}

INLINE u32 conditional_cache_read_word(bool cached, u64 vaddr, u32 paddr) {
    if (cached) {
        return cache_read_word(vaddr, paddr);
    } else {
        return n64_read_physical_word(paddr);
    }
}

INLINE void conditional_cache_write_word(bool cached, u64 vaddr, u32 paddr, u32 value) {
    if (cached) {
        cache_write_word(vaddr, paddr, value);
    } else {
        n64_write_physical_word(paddr, value);
    }
}

INLINE u64 conditional_cache_read_dword(bool cached, u64 vaddr, u32 paddr) {
    if (cached) {
        return cache_read_dword(vaddr, paddr);
    } else {
        return n64_read_physical_dword(paddr);
    }
}

INLINE void conditional_cache_write_dword(bool cached, u64 vaddr, u32 paddr, u64 value) {
    if (cached) {
        cache_write_dword(vaddr, paddr, value);
    } else {
        n64_write_physical_dword(paddr, value);
    }
}
#else
#define conditional_cache_read_byte(cached, vaddr, paddr) n64_read_physical_byte(paddr)
#define conditional_cache_write_byte(cached, vaddr, paddr, value) n64_write_physical_byte(paddr, value)
#define conditional_cache_read_half(cached, vaddr, paddr) n64_read_physical_half(paddr)
#define conditional_cache_write_half(cached, vaddr, paddr, value) n64_write_physical_half(paddr, value)
#define conditional_cache_read_word(cached, vaddr, paddr) n64_read_physical_word(paddr)
#define conditional_cache_write_word(cached, vaddr, paddr, value) n64_write_physical_word(paddr, value)
#define conditional_cache_read_dword(cached, vaddr, paddr) n64_read_physical_dword(paddr)
#define conditional_cache_write_dword(cached, vaddr, paddr, value) n64_write_physical_dword(paddr, value)
#endif



#endif //N64_N64BUS_H
