#include "n64bus.h"

#include <interface/vi.h>
#include <interface/ai.h>
#include <cpu/rsp_interface.h>
#include <rdp/rdp.h>
#ifdef N64_DYNAREC_ENABLED
#include <cpu/dynarec/dynarec.h>
#endif
#include <rsp.h>
#include <interface/si.h>
#include <interface/pi.h>

#include "addresses.h"
#include "pif.h"
#include "mem_util.h"
#include "backup.h"

INLINE u64 get_vpn(u64 address, u32 page_mask_raw) {
    u64 page_mask = page_mask_raw | 0x1FFF;
    // bits 40 and 41: bits 62 and 63 of the address, the "region"
    // bits 0 - 39: the low 40 bits of the address, the actual location being accessed.
    u64 vpn = (address & 0xFFFFFFFFFF) | ((address >> 22) & 0x30000000000);

    // This function is also called for entry_hi, the low 8 bits of which are the ASID
    // this is fine, this mask will take care of that.
    vpn &= ~page_mask;
    return vpn;
}

/* Keeping this in case I need it again
void dump_tlb(u64 vaddr) {
    printf("TLB error at address %016" PRIX64 " and PC %016" PRIX64 ", dumping TLB state:\n\n", vaddr, N64CPU.pc);
    printf("   entry VPN  vaddr VPN  page size  lo0 valid  lo1 valid\n");
    for (int i = 0; i < 32; i++) {
        tlb_entry_t entry = N64CP0.tlb[i];
        word mask = (entry.page_mask.mask << 12) | 0x0FFF;
        word page_size = mask + 1;
        word entry_vpn = get_vpn(entry.entry_hi.raw, entry.page_mask.raw);
        word vaddr_vpn = get_vpn(vaddr, entry.page_mask.raw);

        printf("%02d %08X   %08X   %08X   %d          %d\n", i, entry_vpn, vaddr_vpn, page_size, entry.entry_lo0.valid, entry.entry_lo1.valid);
    }
}
*/

tlb_entry_t* find_tlb_entry(u64 vaddr, int* entry_number) {
    for (int i = 0; i < 32; i++) {
        tlb_entry_t *entry = &N64CP0.tlb[i];
        if (entry->initialized) {
            u64 entry_vpn = get_vpn(entry->entry_hi.raw, entry->page_mask.raw);
            u64 vaddr_vpn = get_vpn(vaddr, entry->page_mask.raw);

            bool vpn_match = entry_vpn == vaddr_vpn;
            bool asid_match = entry->global || (N64CP0.entry_hi.asid == entry->entry_hi.asid);

            if (vpn_match && asid_match) {
                if (entry_number) {
                    *entry_number = i;
                }
                return entry;
            }
        }
    }
    return NULL;
}

bool tlb_probe_slow(u64 vaddr, bus_access_t bus_access, bool* cached, u32* paddr, int* entry_number, bool* dirty) {
    tlb_entry_t* entry = find_tlb_entry(vaddr, entry_number);
    if (!entry) {
        N64CP0.tlb_error = TLB_ERROR_MISS;
        return false;
    }

    u32 mask = (entry->page_mask.mask << 12) | 0x0FFF;
    u32 page_size = mask + 1;
    u32 odd = vaddr & page_size;
    u32 pfn;

    if (!odd) {
        if (!(entry->entry_lo0.valid)) {
            N64CP0.tlb_error = TLB_ERROR_INVALID;
            return false;
        }
        if (bus_access == BUS_STORE && !(entry->entry_lo0.dirty)) {
            N64CP0.tlb_error = TLB_ERROR_MODIFICATION;
            return false;
        }
        if (dirty) {
            *dirty = entry->entry_lo0.dirty;
        }
        pfn = entry->entry_lo0.pfn;
        *cached = entry->entry_lo0.c != 2;
    } else {
        if (!(entry->entry_lo1.valid)) {
            N64CP0.tlb_error = TLB_ERROR_INVALID;
            return false;
        }
        if (bus_access == BUS_STORE && !(entry->entry_lo1.dirty)) {
            N64CP0.tlb_error = TLB_ERROR_MODIFICATION;
            return false;
        }
        if (dirty) {
            *dirty = entry->entry_lo1.dirty;
        }
        pfn = entry->entry_lo1.pfn;
        *cached = entry->entry_lo0.c != 2;
    }

    if (paddr != NULL) {
        *paddr = (pfn << 12) | (vaddr & mask);
    }

    return true;
}

bool tlb_probe_from_cache_entry(tlb_cache_entry_t* entry, u64 vaddr, bus_access_t bus_access, bool* cached, u32* paddr, int* entry_number) {
    if (!entry->hit || (bus_access == BUS_STORE && !entry->dirty)) {
        // Fall back to slow path for misses (for now?)
        bool hit = tlb_probe_slow(vaddr, bus_access, cached, NULL, NULL, NULL);
        if (hit) {
            logfatal("Vaddr %016" PRIX64 " missed when it should have hit!\n", vaddr);
        }
        return hit;
    }

    if (paddr) {
        u32 resolved = entry->base_address + GET_TLB_CACHE_BLOCK_OFFSET(vaddr);
        *paddr = resolved;
    }
    if (entry_number) {
        *entry_number = entry->entry_num;
    }

    *cached = entry->cached;

    return entry->hit;
}

bool tlb_probe(u64 vaddr, bus_access_t bus_access, bool* cached, u32* paddr, int* entry_number) {
    u64 index = GET_TLB_CACHE_INDEX(vaddr);
    u64 tag = GET_TLB_CACHE_TAG(vaddr);
    u64 base_address = GET_TLB_CACHE_BASE_ADDRESS(vaddr);
    u8 asid = N64CP0.entry_hi.asid;

    bool any_invalid = false;

    tlb_cache_entry_t* entries = N64CP0.tlb_cache[index];

    for (int i = 0; i < TLB_CACHE_ASSOCIATIVITY; i++) {
        if (entries[i].valid) {
            bool asid_match = entries[i].global || entries[i].asid == asid;
            if (entries[i].tag == tag && asid_match) {
                return tlb_probe_from_cache_entry(&entries[i], vaddr, bus_access, cached, paddr, entry_number);
            }
        } else {
            any_invalid = true;
        }
    }

    // Didn't find it in the cache, fall back to the slow path:
    int entry_number_cache;
    u32 paddr_cache;
    bool dirty;

    bool hit = tlb_probe_slow(base_address, bus_access, cached, &paddr_cache, &entry_number_cache, &dirty);

    tlb_cache_entry_t* new_dest = NULL;
    if (any_invalid) {
        new_dest = N64CP0.tlb_cache[index];

        // Should be safe, since any_invalid is set to true, there will be one.
        while (new_dest->valid) {
            new_dest++;
        }
    } else {
        new_dest = &N64CP0.tlb_cache[index][rand() % TLB_CACHE_ASSOCIATIVITY];
    }

    memset(new_dest, 0, sizeof(tlb_cache_entry_t));
    new_dest->valid = true;
    new_dest->hit = hit;
    new_dest->dirty = hit ? dirty : false;
    new_dest->cached = *cached;

    new_dest->entry_num = hit ? entry_number_cache : 0;
    new_dest->tag = tag;
    new_dest->base_address = paddr_cache;
    new_dest->asid = asid;

    new_dest->global = hit ? N64CP0.tlb[entry_number_cache].global : true; // set it as global if it's a miss, so we'll always find it

    return tlb_probe_from_cache_entry(new_dest, vaddr, bus_access, cached, paddr, entry_number);
}

u32 read_word_rdramreg(u32 address) {
    if (address % 4 != 0) {
        logfatal("Reading from RDRAM register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RDRAM_REG_FIRST || address > ADDR_RDRAM_REG_LAST) {
        logfatal("In RDRAM register read handler with out of bounds address 0x%08X", address);
    }
    switch (address) {
        case ADDR_RDRAM_CONFIG_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_CONFIG_REG");
        case ADDR_RDRAM_DEVICE_ID_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_DEVICE_ID_REG");
        case ADDR_RDRAM_DELAY_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_DELAY_REG");
        case ADDR_RDRAM_MODE_REG:
            logwarn("Read from RDRAM_MODE_REG, returning 0");
            return 0;
        case ADDR_RDRAM_REF_INTERVAL_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_REF_INTERVAL_REG");
        case ADDR_RDRAM_REF_ROW_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_REF_ROW_REG");
        case ADDR_RDRAM_RAS_INTERVAL_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_RAS_INTERVAL_REG");
        case ADDR_RDRAM_MIN_INTERVAL_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_MIN_INTERVAL_REG");
        case ADDR_RDRAM_ADDR_SELECT_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_ADDR_SELECT_REG");
        case ADDR_RDRAM_DEVICE_MANUF_REG:
            logfatal("Read from unimplemented RDRAM reg: ADDR_RDRAM_DEVICE_MANUF_REG");
        default:
            logfatal("Read from unknown RDRAM reg: 0x%08X", address);
    }
}

void write_word_rdramreg(u32 address, u32 value) {
    if (address % 4 != 0) {
        logfatal("Writing to RDRAM register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RDRAM_REG_FIRST || address > ADDR_RDRAM_REG_LAST) {
        logwarn("In RDRAM register write handler with out of bounds address 0x%08X", address);
    } else {
        n64sys.mem.rdram_reg[(address - SREGION_RDRAM_REGS) / 4] = value;
    }
}

u32 read_word_rireg(u32 address) {
    if (address % 4 != 0) {
        logfatal("Reading from RI register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RI_MODE_REG || address > ADDR_RI_WERROR_REG) {
        logfatal("In RI read handler with out of bounds address 0x%08X", address);
    }

    u32 value = n64sys.mem.ri_reg[(address - SREGION_RI_REGS) / 4];
    logwarn("Reading RI reg, returning 0x%08X", value);
    return value;
}

void write_word_rireg(u32 address, u32 value) {
    if (address % 4 != 0) {
        logfatal("Writing to RI register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RI_FIRST || address > ADDR_RI_LAST) {
        logfatal("In RI write handler with out of bounds address 0x%08X", address);
    }

    logwarn("Writing RI reg with value 0x%08X", value);
    n64sys.mem.ri_reg[(address - SREGION_RI_REGS) / 4] = value;
}

void write_word_mireg(u32 address, u32 value) {
    if (address % 4 != 0) {
        logfatal("Writing to MI register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_MI_FIRST || address > ADDR_MI_LAST) {
        logfatal("In MI write handler with out of bounds address 0x%08X", address);
    }
    switch (address) {
        case ADDR_MI_MODE_REG: {
            // Lowest 7 bits: 'init length'
            n64sys.mi.init_mode &= 0xFFFFFF80;
            n64sys.mi.init_mode |= value & 0x7F;

            // Bit 7 = "clear init mode"
            if (value & (1 << 7)) {
                n64sys.mi.init_mode &= ~(1 << 7);
            }

            // Bit 8 = "set init mode"
            if (value & (1 << 8)) {
                n64sys.mi.init_mode |= 1 << 7;
            }

            // Bit 9 = "clear ebus test mode"
            if (value & (1 << 9)) {
                n64sys.mi.init_mode &= ~(1 << 8);
            }

            // Bit 10 = "set ebus test mode"
            if (value & (1 << 10)) {
                n64sys.mi.init_mode |= 1 << 8;
            }

            // Bit 11 = "clear DP interrupt"
            if (value & (1 << 11)) {
                interrupt_lower(INTERRUPT_DP);
            }

            // Bit 12 = "clear RDRAM reg"
            if (value & (1 << 12)) {
                n64sys.mi.init_mode &= ~(1 << 9);
            }

            // Bit 13 = "set RDRAM reg mode"
            if (value & (1 << 13)) {
                n64sys.mi.init_mode |= 1 << 9;
            }
            break;
        }
        case ADDR_MI_VERSION_REG:
            logwarn("Ignoring write to MI version reg!");
            break;
        case ADDR_MI_INTR_REG:
            logwarn("Ignoring write to MI intr reg!");
            break;
        case ADDR_MI_INTR_MASK_REG:
            for (int bit = 0; bit < 6; bit++) {
                int clearbit = bit * 2;
                int setbit = (bit * 2) + 1;

                // Clear
                if (value & (1 << clearbit)) {
                    n64sys.mi.intr_mask.raw &= ~(1 << bit);
                }
                // Set
                if (value & (1 << setbit)) {
                    n64sys.mi.intr_mask.raw |= 1 << bit;
                }
            }
            on_interrupt_change();
            break;
        default:
            logfatal("Write to unknown MI register: 0x%08X", address);
    }
}

u32 read_word_mireg(u32 address) {
    if (address % 4 != 0) {
        logfatal("Reading from MI register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_MI_FIRST || address > ADDR_MI_LAST) {
        logfatal("In MI read handler with out of bounds address 0x%08X", address);
    }

    switch (address) {
        case ADDR_MI_MODE_REG:
            logfatal("Read from unimplemented MI register: ADDR_MI_MODE_REG");
        case ADDR_MI_VERSION_REG:
            return 0x02020102;
        case ADDR_MI_INTR_REG:
            return n64sys.mi.intr.raw;
        case ADDR_MI_INTR_MASK_REG:
            return n64sys.mi.intr_mask.raw;
        default:
            logfatal("Read from unknown MI register: 0x%08X", address);
    }
}

u32 read_unused(u32 address) {
    logwarn("Reading unused value at 0x%08X!", address);
    return 0;
}

void n64_write_physical_dword(u32 address, u64 value) {
    if (address & 0b111) {
        logfatal("Tried to write to unaligned DWORD");
    }
    logdebug("Writing 0x%016" PRIX64 " to [0x%08X]", value, address);
#ifdef N64_DYNAREC_ENABLED
    invalidate_dynarec_page(address);
#endif
    switch (address) {
        case REGION_RDRAM:
            dword_to_byte_array((u8*) &n64sys.mem.rdram, DWORD_ADDRESS(address) - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_MEM: {
            value >>= 32; // TODO: this is probably wrong, it probably depends on the address.
            if (address & 0x1000) {
                word_to_byte_array((u8*) &N64RSP.sp_imem, DWORD_ADDRESS(address & 0xFFF), value);
                invalidate_rsp_icache(DWORD_ADDRESS(address));
            } else {
                word_to_byte_array((u8*) &N64RSP.sp_dmem, address & 0xFFF, htobe32(value));
            }
            break;
        }
        case REGION_SP_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_SP_REGS", value, address);
            break;
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address);
        case REGION_DP_SPAN_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address);
        case REGION_MI_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_MI_REGS", value, address);
            break;
        case REGION_VI_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_VI_REGS", value, address);
            break;
        case REGION_AI_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_AI_REGS", value, address);
            break;
        case REGION_PI_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_PI_REGS", value, address);
            break;
        case REGION_RI_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_RI_REGS", value, address);
            break;
        case REGION_SI_REGS:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_SI_REGS", value, address);
            break;
        case REGION_UNUSED:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_UNUSED", value, address);
        case REGION_CART:
            write_dword_pibus(address, value);
            break;
        case REGION_PIF_BOOT:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address);
        case REGION_PIF_RAM:
            dword_to_byte_array(n64sys.mem.pif_ram, address - SREGION_PIF_RAM, htobe64(value));
            process_pif_command();
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in region: REGION_PIF_RAM", value, address);
            break;
        case REGION_RESERVED:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_RESERVED", value, address);
        case REGION_CART_1_3:
            logfatal("Writing dword 0x%016" PRIX64 " to address 0x%08X in unsupported region: REGION_CART_1_3", value, address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
            break;
        default:
            logfatal("Writing dword 0x%016" PRIX64 " to unknown address: 0x%08X", value, address);
    }
}

u64 n64_read_physical_dword(u32 address) {
    if (address & 0b111) {
        logfatal("Tried to load from unaligned DWORD");
    }
    switch (address) {
        case REGION_RDRAM:
            return dword_from_byte_array((u8*) &n64sys.mem.rdram, DWORD_ADDRESS(address) - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(DWORD_ADDRESS(address));
        case REGION_RDRAM_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_MEM:
            if (address & 0x1000) {
                return dword_from_byte_array((u8*) &N64RSP.sp_imem, DWORD_ADDRESS(address & 0xFFF));
            } else {
                return be64toh(dword_from_byte_array((u8*) &N64RSP.sp_dmem, address & 0xFFF));
            }
        case REGION_SP_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_SP_REGS", address);
        case REGION_DP_COMMAND_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
        case REGION_DP_SPAN_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address);
        case REGION_MI_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_MI_REGS", address);
        case REGION_VI_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_VI_REGS", address);
        case REGION_AI_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_AI_REGS", address);
        case REGION_PI_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_PI_REGS", address);
        case REGION_RI_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_RI_REGS", address);
        case REGION_SI_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_SI_REGS", address);
        case REGION_UNUSED:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_UNUSED", address);
        case REGION_CART:
            return read_dword_pibus(address);
        case REGION_PIF_BOOT:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_PIF_BOOT", address);
        case REGION_PIF_RAM:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_PIF_RAM", address);
        case REGION_RESERVED:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_RESERVED", address);
        case REGION_CART_1_3:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_CART_1_3", address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
        default:
            logfatal("Reading dword from unknown address: 0x%08X", address);
    }
}


void n64_write_physical_word(u32 address, u32 value) {
    if (address & 0b11) {
        logfatal("Tried to write to unaligned WORD");
    }
    logdebug("Writing 0x%08X to [0x%08X]", value, address);
#ifdef N64_DYNAREC_ENABLED
    invalidate_dynarec_page(WORD_ADDRESS(address));
#endif
    switch (address) {
        case REGION_RDRAM:
            word_to_byte_array((u8*) &n64sys.mem.rdram, WORD_ADDRESS(address) - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            write_word_rdramreg(address, value);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_MEM:
            if (address & 0x1000) {
                word_to_byte_array((u8*) &N64RSP.sp_imem, WORD_ADDRESS(address & 0xFFF), value);
                invalidate_rsp_icache(WORD_ADDRESS(address));
            } else {
                word_to_byte_array((u8 *) &N64RSP.sp_dmem, address & 0xFFF, htobe32(value));
            }
            break;
        case REGION_SP_REGS:
            write_word_spreg(address, value);
            break;
        case REGION_DP_COMMAND_REGS:
            write_word_dpcreg(address, value);
            break;
        case REGION_DP_SPAN_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address);
        case REGION_MI_REGS:
            write_word_mireg(address, value);
            break;
        case REGION_VI_REGS:
            write_word_vireg(address, value);
            break;
        case REGION_AI_REGS:
            write_word_aireg(address, value);
            break;
        case REGION_PI_REGS:
            write_word_pireg(address, value);
            break;
        case REGION_RI_REGS:
            write_word_rireg(address, value);
            break;
        case REGION_SI_REGS:
            write_word_sireg(address, value);
            break;
        case REGION_UNUSED:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_UNUSED", value, address);
        case REGION_CART:
            write_word_pibus(address, value);
            break;
        case REGION_PIF_BOOT:
            logwarn("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address);
            break;
        case REGION_PIF_RAM:
            word_to_byte_array(n64sys.mem.pif_ram, address - SREGION_PIF_RAM, htobe32(value));
            process_pif_command();
            logwarn("Writing word 0x%08X to address 0x%08X in region: REGION_PIF_RAM", value, address);
            break;
        case REGION_RESERVED:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_RESERVED", value, address);
        case REGION_CART_1_3:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_3", value, address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
            break;
        default:
            logfatal("Writing word 0x%08X to unknown address: 0x%08X", value, address);
    }
}

u32 n64_read_physical_word(u32 address) {
    if (address & 0b11) {
        logfatal("Tried to load from unaligned WORD");
    }
    switch (address) {
        case REGION_RDRAM:
            return word_from_byte_array((u8*) &n64sys.mem.rdram, WORD_ADDRESS(address) - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(address);
        case REGION_RDRAM_REGS:
            return read_word_rdramreg(address);
        case REGION_SP_MEM:
            if (address & 0x1000) {
                return word_from_byte_array((u8*) &N64RSP.sp_imem, WORD_ADDRESS(address & 0xFFF));
            } else {
                return be32toh(word_from_byte_array((u8*) &N64RSP.sp_dmem, address & 0xFFF));
            }
        case REGION_SP_REGS:
            return read_word_spreg(address);
        case REGION_DP_COMMAND_REGS:
            return read_word_dpcreg(address);
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
        case REGION_DP_SPAN_REGS:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address);
        case REGION_MI_REGS:
            return read_word_mireg(address);
        case REGION_VI_REGS:
            return read_word_vireg(address);
        case REGION_AI_REGS:
            return read_word_aireg(address);
        case REGION_PI_REGS:
            return read_word_pireg(address);
        case REGION_RI_REGS:
            return read_word_rireg(address);
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_RI_REGS", address);
        case REGION_SI_REGS:
            return read_word_sireg(address);
        case REGION_UNUSED:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_UNUSED", address);
        case REGION_CART:
            return read_word_pibus(address);
        case REGION_PIF_BOOT: {
            if (n64sys.mem.rom.pif_rom == NULL) {
                logfatal("Tried to read from PIF ROM, but PIF ROM not loaded!\n");
            } else {
                u32 index = address - SREGION_PIF_BOOT;
                if (index > n64sys.mem.rom.size - 3) { // -3 because we're reading an entire word
                    logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the PIF ROM!", address, index, index);
                } else {
                    return be32toh(word_from_byte_array(n64sys.mem.rom.pif_rom, index));
                }
            }
        }
        case REGION_PIF_RAM: {
            return be32toh(word_from_byte_array(n64sys.mem.pif_ram, address - SREGION_PIF_RAM));
        }
        case REGION_RESERVED:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_RESERVED", address);
        case REGION_CART_1_3:
            logwarn("Reading word from address 0x%08X in unsupported region: REGION_CART_1_3", address);
            return 0;
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
        default:
            logfatal("Reading word from unknown address: 0x%08X", address);
    }
}

// Handle the bus edge for 16 bit writes to PIF and SPMEM
INLINE u32 bus_edge_case_half_pif_spmem(u32 address, u32 value) {
    // Write to address & ~3.
    // 4-byte aligned: write value in upper bytes, zeroes in lower bytes
    // 2-byte aligned: write full 32 bit value

    // Bit hacks to avoid an if statement here.
    // !(address & 2) gives a 0 if the address is 2 byte aligned, and a 1 if it's 4 byte aligned.
    return value << (16 * !(address & 2));
}

void n64_write_physical_half(u32 address, u32 value) {
    if (address & 0b1) {
        logfatal("Tried to write to unaligned HALF");
    }
    logdebug("Writing 0x%04X to [0x%08X]", value & 0xFFFF, address);
#ifdef N64_DYNAREC_ENABLED
    invalidate_dynarec_page(HALF_ADDRESS(address));
#endif
    switch (address) {
        case REGION_RDRAM:
            half_to_byte_array((u8*) &n64sys.mem.rdram, HALF_ADDRESS(address) - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value & 0xFFFF, address);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_MEM:
            value = bus_edge_case_half_pif_spmem(address, value);
            address &= ~3;
            if (address & 0x1000) {
                word_to_byte_array((u8*) &N64RSP.sp_imem, address & 0xFFF, value);
                invalidate_rsp_icache(address);
            } else {
                word_to_byte_array((u8*) &N64RSP.sp_dmem, address & 0xFFF, htobe32(value));
            }
            break;
        case REGION_SP_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_SP_REGS", value & 0xFFFF, address);
            break;
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value & 0xFFFF, address);
        case REGION_DP_SPAN_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value & 0xFFFF, address);
        case REGION_MI_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_MI_REGS", value & 0xFFFF, address);
            break;
        case REGION_VI_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_VI_REGS", value & 0xFFFF, address);
            break;
        case REGION_AI_REGS:
            logwarn("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_AI_REGS", value & 0xFFFF, address);
            break;
        case REGION_PI_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_PI_REGS", value & 0xFFFF, address);
            break;
        case REGION_RI_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_RI_REGS", value & 0xFFFF, address);
            break;
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_RI_REGS", value & 0xFFFF, address);
        case REGION_SI_REGS:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_SI_REGS", value & 0xFFFF, address);
            break;
        case REGION_UNUSED:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_UNUSED", value & 0xFFFF, address);
        case REGION_CART:
            write_half_pibus(address, value);
            break;
        case REGION_PIF_BOOT:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value & 0xFFFF, address);
        case REGION_PIF_RAM: {
            value = bus_edge_case_half_pif_spmem(address, value);
            address &= ~3;
            word_to_byte_array((u8 *) &n64sys.mem.pif_ram, address - SREGION_PIF_RAM, htobe32(value));
            process_pif_command();
            break;
        }
        case REGION_RESERVED:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_RESERVED", value & 0xFFFF, address);
        case REGION_CART_1_3:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_CART_1_3", value & 0xFFFF, address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
            break;
        default:
            logfatal("Writing u16 0x%04X to unknown address: 0x%08X", value & 0xFFFF, address);
    }
}

u16 n64_read_physical_half(u32 address) {
    if (address & 0b1) {
        logfatal("Tried to load from unaligned HALF");
    }
    switch (address) {
        case REGION_RDRAM:
            return half_from_byte_array((u8*) &n64sys.mem.rdram, HALF_ADDRESS(address) - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(address);
        case REGION_RDRAM_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_MEM:
            if (address & 0x1000) {
                return half_from_byte_array((u8*) &N64RSP.sp_imem, HALF_ADDRESS(address & 0xFFF));
            } else {
                return be16toh(half_from_byte_array((u8*) &N64RSP.sp_dmem, address & 0xFFF));
            }
        case REGION_SP_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_SP_REGS", address);
        case REGION_DP_COMMAND_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
        case REGION_DP_SPAN_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address);
        case REGION_MI_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_MI_REGS", address);
        case REGION_VI_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_VI_REGS", address);
        case REGION_AI_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_AI_REGS", address);
        case REGION_PI_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_PI_REGS", address);
        case REGION_RI_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_RI_REGS", address);
        case REGION_SI_REGS:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_SI_REGS", address);
        case REGION_UNUSED:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_UNUSED", address);
        case REGION_CART:
            return read_half_pibus(address);
        case REGION_PIF_BOOT:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_PIF_BOOT", address);
        case REGION_PIF_RAM:
            return be16toh(half_from_byte_array(n64sys.mem.pif_ram, address - SREGION_PIF_RAM));
        case REGION_RESERVED:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_RESERVED", address);
        case REGION_CART_1_3:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_CART_1_3", address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
        default:
            logfatal("Reading u16 from unknown address: 0x%08X", address);
    }
}

void n64_write_physical_byte(u32 address, u32 value) {
    logdebug("Writing 0x%02X to [0x%08X]", value & 0xFF, address);
#ifdef N64_DYNAREC_ENABLED
    invalidate_dynarec_page(BYTE_ADDRESS(address));
#endif
    switch (address) {
        case REGION_RDRAM:
            n64sys.mem.rdram[BYTE_ADDRESS(address)] = value;
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value & 0xFF, address);
        case REGION_SP_MEM:
            value = value << (8 * (3 - (address & 3)));
            address = (address & 0xFFF) & ~3;
            if (address & 0x1000) {
                word_to_byte_array(N64RSP.sp_imem, address & 0xFFF, value);
                invalidate_rsp_icache(address);
            } else {
                word_to_byte_array(N64RSP.sp_dmem, address & 0xFFF, htobe32(value));
            }
            break;
        case REGION_SP_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_SP_REGS", value & 0xFF, address);
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value & 0xFF, address);
        case REGION_DP_SPAN_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value & 0xFF, address);
        case REGION_MI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_MI_REGS", value & 0xFF, address);
        case REGION_VI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_VI_REGS", value & 0xFF, address);
        case REGION_AI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_AI_REGS", value & 0xFF, address);
        case REGION_PI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_PI_REGS", value & 0xFF, address);
        case REGION_RI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RI_REGS", value & 0xFF, address);
        case REGION_SI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_SI_REGS", value & 0xFF, address);
        case REGION_UNUSED:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_UNUSED", value & 0xFF, address);
        case REGION_CART:
            write_byte_pibus(address, value);
            break;
        case REGION_PIF_BOOT:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value & 0xFF, address);
        case REGION_PIF_RAM: {
            // Seems to be different (kinda the opposite) behavior as SH to PIF RAM, weirdly enough
            value = value << (8 * (3 - (address & 3)));
            address = (address - SREGION_PIF_RAM) & ~3;
            word_to_byte_array((u8*) &n64sys.mem.pif_ram, address, htobe32(value));
            process_pif_command();
            break;
        }
        case REGION_RESERVED:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RESERVED", value & 0xFF, address);
        case REGION_CART_1_3:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_3", value & 0xFF, address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
        default:
            logfatal("Writing byte 0x%02X to unknown address: 0x%08X", value & 0xFF, address);
    }
}

u8 n64_read_physical_byte(u32 address) {
    switch (address) {
        case REGION_RDRAM:
            return n64sys.mem.rdram[BYTE_ADDRESS(address)];
        case REGION_RDRAM_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_MEM:
            if (address & 0x1000) {
                return N64RSP.sp_imem[BYTE_ADDRESS(address) - SREGION_SP_IMEM];
            } else {
                return N64RSP.sp_dmem[address - SREGION_SP_DMEM];
            }
        case REGION_SP_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_SP_REGS", address);
        case REGION_DP_COMMAND_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
        case REGION_DP_SPAN_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address);
        case REGION_MI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_MI_REGS", address);
        case REGION_VI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_VI_REGS", address);
        case REGION_AI_REGS: {
            u32 w = read_word_aireg(address & (~3));
            int offset = 3 - (address & 3);
            return (w >> (offset * 8)) & 0xFF;
        }
        case REGION_PI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_PI_REGS", address);
        case REGION_RI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RI_REGS", address);
        case REGION_SI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_SI_REGS", address);
        case REGION_UNUSED:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_UNUSED", address);
        case REGION_CART:
            return read_byte_pibus(address);
        case REGION_PIF_BOOT: {
            if (n64sys.mem.rom.pif_rom == NULL) {
                logfatal("Tried to read from PIF ROM, but PIF ROM not loaded!\n");
            } else {
                u32 index = address - SREGION_PIF_BOOT;
                if (index > n64sys.mem.rom.size) {
                    logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the PIF ROM!", address, index, index);
                } else {
                    return n64sys.mem.rom.pif_rom[index];
                }
            }
        }
        case REGION_PIF_RAM:
            return n64sys.mem.pif_ram[address - SREGION_PIF_RAM];
        case REGION_RESERVED:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RESERVED", address);
        case REGION_CART_1_3:
            logwarn("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_3", address);
            return 0;
        case REGION_SYSAD_DEVICE:
            logfatal("This (0x%08X) is a virtual address!", address);
        default:
            logfatal("Reading byte from unknown address: 0x%08X", address);
    }
}
