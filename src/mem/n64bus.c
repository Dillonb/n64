#include "n64bus.h"

#include <interface/vi.h>
#include <interface/ai.h>
#include <cpu/rsp_interface.h>
#include <rdp/rdp.h>
#include <cpu/dynarec/dynarec.h>

#include "addresses.h"
#include "pif.h"
#include "mem_util.h"
#include "backup.h"

dword get_vpn(dword address, word page_mask_raw) {
    dword tmp = page_mask_raw | 0x1FFF;
    dword vpn = address & ~tmp;

    return vpn;
}

bool tlb_probe(dword vaddr, word* paddr, int* entry_number, cp0_t* cp0) {
    for (int i = 0; i < 32; i++) {
        tlb_entry_t entry = cp0->tlb[i];
        word mask = (entry.page_mask.mask << 12) | 0x0FFF;
        word page_size = mask + 1;
        word entry_vpn = get_vpn(entry.entry_hi.raw, entry.page_mask.raw);
        word vaddr_vpn = get_vpn(vaddr, entry.page_mask.raw);

        if (entry_vpn != vaddr_vpn) {
            continue;
        }

        word odd = vaddr & page_size;
        word pfn;

        if (!odd) {
            if (!(entry.entry_lo0.valid)) {
                continue;
            }
            pfn = entry.entry_lo0.pfn;
        } else {
            if (!(entry.entry_lo1.valid)) {
                continue;
            }
            pfn = entry.entry_lo1.pfn;
        }

        if (paddr != NULL) {
            *paddr = (pfn << 12) | (vaddr & mask);
        }
        if (entry_number != NULL) {
            *entry_number = i;
        }
        return true;
    }
    return false;
}

bool tlb_probe_64(dword vaddr, word* paddr, int* entry_number, cp0_t* cp0) {
    for (int i = 0; i < 32; i++) {
        tlb_entry_64_t entry = cp0->tlb_64[i];
        word mask = (entry.page_mask.mask << 12) | 0x0FFF;
        word page_size = mask + 1;
        word entry_vpn = get_vpn(entry.entry_hi.raw, entry.page_mask.raw);
        word vaddr_vpn = get_vpn(vaddr, entry.page_mask.raw);

        if (entry_vpn != vaddr_vpn) {
            continue;
        }

        word odd = vaddr & page_size;
        word pfn;

        if (!odd) {
            if (!(entry.entry_lo0.valid)) {
                continue;
            }
            pfn = entry.entry_lo0.pfn;
        } else {
            if (!(entry.entry_lo1.valid)) {
                continue;
            }
            pfn = entry.entry_lo1.pfn;
        }

        if (paddr != NULL) {
            *paddr = (pfn << 12) | (vaddr & mask);
        }
        if (entry_number != NULL) {
            *entry_number = i;
        }
        return true;
    }
    return false;
}

word read_word_rdramreg(n64_system_t* system, word address) {
    if (address % 4 != 0) {
        logfatal("Reading from RDRAM register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RDRAM_REG_FIRST || address > ADDR_RDRAM_REG_LAST) {
        logfatal("In RDRAM register write handler with out of bounds address 0x%08X", address);
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

void write_word_rdramreg(n64_system_t* system, word address, word value) {
    if (address % 4 != 0) {
        logfatal("Writing to RDRAM register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RDRAM_REG_FIRST || address > ADDR_RDRAM_REG_LAST) {
        logwarn("In RDRAM register write handler with out of bounds address 0x%08X", address);
    } else {
        system->mem.rdram_reg[(address - SREGION_RDRAM_REGS) / 4] = value;
    }
}

word read_word_pireg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_PI_DRAM_ADDR_REG:
            return system->mem.pi_reg[PI_DRAM_ADDR_REG];
        case ADDR_PI_CART_ADDR_REG:
            return system->mem.pi_reg[PI_CART_ADDR_REG];
        case ADDR_PI_RD_LEN_REG:
            return system->mem.pi_reg[PI_RD_LEN_REG];
        case ADDR_PI_WR_LEN_REG:
            return system->mem.pi_reg[PI_WR_LEN_REG];
        case ADDR_PI_STATUS_REG: {
            word value = 0;
            value |= (0 << 0); // Is PI DMA active?
            value |= (0 << 1); // Is PI IO busy?
            value |= (0 << 2); // PI IO error?
            value |= (system->mi.intr.pi << 3); // PI interrupt?
            return value;
        }
        case ADDR_PI_DOMAIN1_REG:
            return system->mem.pi_reg[PI_DOMAIN1_REG];
        case ADDR_PI_BSD_DOM1_PWD_REG:
            return system->mem.pi_reg[PI_BSD_DOM1_PWD_REG];
        case ADDR_PI_BSD_DOM1_PGS_REG:
            return system->mem.pi_reg[PI_BSD_DOM1_PGS_REG];
        case ADDR_PI_BSD_DOM1_RLS_REG:
            return system->mem.pi_reg[PI_BSD_DOM1_RLS_REG];
        case ADDR_PI_DOMAIN2_REG:
            return system->mem.pi_reg[PI_DOMAIN2_REG];
        case ADDR_PI_BSD_DOM2_PWD_REG:
            return system->mem.pi_reg[PI_BSD_DOM2_PWD_REG];
        case ADDR_PI_BSD_DOM2_PGS_REG:
            return system->mem.pi_reg[PI_BSD_DOM2_PGS_REG];
        case ADDR_PI_BSD_DOM2_RLS_REG:
            return system->mem.pi_reg[PI_BSD_DOM2_RLS_REG];
        default:
            logfatal("Reading word from unknown PI register 0x%08X", address);
    }
}

void write_word_pireg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_PI_DRAM_ADDR_REG:
            system->mem.pi_reg[PI_DRAM_ADDR_REG] = value & ~1;
            if (system->mem.pi_reg[PI_DRAM_ADDR_REG] != value) {
                logwarn("Misaligned PI_DRAM_ADDR! 0x%08X force-aligned to 0x%08X.", value, system->mem.pi_reg[PI_DRAM_ADDR_REG]);
            }
            break;
        case ADDR_PI_CART_ADDR_REG:
            system->mem.pi_reg[PI_CART_ADDR_REG] = value & ~1;
            if (system->mem.pi_reg[PI_CART_ADDR_REG] != value) {
                logwarn("Misaligned PI_CART_ADDR! 0x%08X force-aligned to 0x%08X.", value, system->mem.pi_reg[PI_CART_ADDR_REG]);
            }
            break;
        case ADDR_PI_RD_LEN_REG: {
            word length = (value & 0x00FFFFFF) + 1;
            if (length & 0x7) {
                length = (length + 0x7) & ~0x7;
            }
            system->mem.pi_reg[PI_RD_LEN_REG] = length;
            word cart_addr = system->mem.pi_reg[PI_CART_ADDR_REG];
            word dram_addr = system->mem.pi_reg[PI_DRAM_ADDR_REG];
            if (cart_addr < SREGION_CART_2_1) {
                logfatal("Cart address too low! 0x%08X masked to 0x%08X\n", system->mem.pi_reg[PI_CART_ADDR_REG], cart_addr);
            }
            if (dram_addr >= SREGION_RDRAM_UNUSED) {
                logfatal("DRAM address too high!");
            }


            logdebug("DMA requested at PC 0x%016lX from 0x%08X to 0x%08X (DRAM to CART), with a length of %d", system->cpu.pc, dram_addr, cart_addr, length);

            for (int i = 0; i < length; i++) {
                byte b = n64_read_byte(system, dram_addr + i);
                logtrace("DRAM to CART: Copying 0x%02X from 0x%08X to 0x%08X", b, dram_addr + i, cart_addr + i);
                n64_write_byte(system, cart_addr + i, b);
            }

            logdebug("DMA completed.");
            system->mem.pi_reg[PI_DRAM_ADDR_REG] += length;
            system->mem.pi_reg[PI_CART_ADDR_REG] += length;
            interrupt_raise(INTERRUPT_PI);
            break;
        }
        case ADDR_PI_WR_LEN_REG: {
            word length = (value & 0x00FFFFFF) + 1;
            if (length & 0x7) {
                length = (length + 0x7) & ~0x7;
            }
            system->mem.pi_reg[PI_WR_LEN_REG] = length;
            word cart_addr = system->mem.pi_reg[PI_CART_ADDR_REG];
            word dram_addr = system->mem.pi_reg[PI_DRAM_ADDR_REG] & 0x7FFFFF;
            if (cart_addr < SREGION_CART_2_1) {
                logfatal("Cart address too low! 0x%08X masked to 0x%08X\n", system->mem.pi_reg[PI_CART_ADDR_REG], cart_addr);
            }

            logdebug("DMA requested at PC 0x%016lX from 0x%08X to 0x%08X (CART to DRAM), with a length of %d", system->cpu.pc, cart_addr, dram_addr, length);

            for (int i = 0; i < length; i++) {
                byte b = n64_read_byte(system, cart_addr + i);
                logtrace("CART to DRAM: Copying 0x%02X from 0x%08X to 0x%08X", b, cart_addr + i, dram_addr + i);
                n64_write_byte(system, dram_addr + i, b);
            }

            logdebug("DMA completed.");
            static bool first_time = true;
            if (first_time) {
                n64_write_word(system, 0x318, N64_RDRAM_SIZE);
                first_time = false;
            }
            system->mem.pi_reg[PI_DRAM_ADDR_REG] += length;
            system->mem.pi_reg[PI_CART_ADDR_REG] += length;
            interrupt_raise(INTERRUPT_PI);
            break;
        }
        case ADDR_PI_STATUS_REG: {
            if (value & 0b10) {
                interrupt_lower(system, INTERRUPT_PI);
            }
            break;
        }
        case ADDR_PI_DOMAIN1_REG:
            system->mem.pi_reg[PI_DOMAIN1_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM1_PWD_REG:
            system->mem.pi_reg[PI_BSD_DOM1_PWD_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM1_PGS_REG:
            system->mem.pi_reg[PI_BSD_DOM1_PGS_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM1_RLS_REG:
            system->mem.pi_reg[PI_BSD_DOM1_RLS_REG] = value & 0xFF;
            break;
        case ADDR_PI_DOMAIN2_REG:
            system->mem.pi_reg[PI_DOMAIN2_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM2_PWD_REG:
            system->mem.pi_reg[PI_BSD_DOM2_PWD_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM2_PGS_REG:
            system->mem.pi_reg[PI_BSD_DOM2_PGS_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM2_RLS_REG:
            system->mem.pi_reg[PI_BSD_DOM2_RLS_REG] = value & 0xFF;
            break;
        default:
            logfatal("Writing word 0x%08X to unknown PI register 0x%08X", value, address);
    }
}

word read_word_rireg(n64_system_t* system, word address) {
    if (address % 4 != 0) {
        logfatal("Reading from RI register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RI_MODE_REG || address > ADDR_RI_WERROR_REG) {
        logfatal("In RI read handler with out of bounds address 0x%08X", address);
    }

    word value = system->mem.ri_reg[(address - SREGION_RI_REGS) / 4];
    logwarn("Reading RI reg, returning 0x%08X", value);
    return value;
}

void write_word_rireg(n64_system_t* system, word address, word value) {
    if (address % 4 != 0) {
        logfatal("Writing to RI register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RI_FIRST || address > ADDR_RI_LAST) {
        logfatal("In RI write handler with out of bounds address 0x%08X", address);
    }

    logwarn("Writing RI reg with value 0x%08X", value);
    system->mem.ri_reg[(address - SREGION_RI_REGS) / 4] = value;
}

void write_word_mireg(n64_system_t* system, word address, word value) {
    if (address % 4 != 0) {
        logfatal("Writing to MI register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_MI_FIRST || address > ADDR_MI_LAST) {
        logfatal("In MI write handler with out of bounds address 0x%08X", address);
    }
    switch (address) {
        case ADDR_MI_MODE_REG: {
            // Lowest 7 bits: 'init length'
            system->mi.init_mode &= 0xFFFFFF80;
            system->mi.init_mode |= value & 0x7F;

            // Bit 7 = "clear init mode"
            if (value & (1 << 7)) {
                system->mi.init_mode &= ~(1 << 7);
            }

            // Bit 8 = "set init mode"
            if (value & (1 << 8)) {
                system->mi.init_mode |= 1 << 7;
            }

            // Bit 9 = "clear ebus test mode"
            if (value & (1 << 9)) {
                system->mi.init_mode &= ~(1 << 8);
            }

            // Bit 10 = "set ebus test mode"
            if (value & (1 << 10)) {
                system->mi.init_mode |= 1 << 8;
            }

            // Bit 11 = "clear DP interrupt"
            if (value & (1 << 11)) {
                interrupt_lower(system, INTERRUPT_DP);
            }

            // Bit 12 = "clear RDRAM reg"
            if (value & (1 << 12)) {
                system->mi.init_mode &= ~(1 << 9);
            }

            // Bit 13 = "set RDRAM reg mode"
            if (value & (1 << 13)) {
                system->mi.init_mode |= 1 << 9;
            }
            break;
        }
        case ADDR_MI_VERSION_REG:
            logwarn("Ignoring write to MI version reg!");
            break;
        case ADDR_MI_INTR_REG:
            logfatal("Unhandled write to ADDR_MI_INTR_REG");
        case ADDR_MI_INTR_MASK_REG:
            for (int bit = 0; bit < 6; bit++) {
                int clearbit = bit * 2;
                int setbit = (bit * 2) + 1;

                // Clear
                if (value & (1 << clearbit)) {
                    system->mi.intr_mask.raw &= ~(1 << bit);
                }
                // Set
                if (value & (1 << setbit)) {
                    system->mi.intr_mask.raw |= 1 << bit;
                }
            }
            on_interrupt_change(system);
            break;
        default:
            logfatal("Write to unknown MI register: 0x%08X", address);
    }
}

word read_word_mireg(n64_system_t* system, word address) {
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
            return system->mi.intr.raw;
        case ADDR_MI_INTR_MASK_REG:
            return system->mi.intr_mask.raw;
        default:
            logfatal("Read from unknown MI register: 0x%08X", address);
    }
}

void pif_to_dram(n64_system_t* system, word pif_address, word dram_address) {
    if ((dram_address & 1) != 0) {
        logfatal("PIF to DRAM on unaligned address");
    }
    unimplemented(pif_address != 0x1FC007C0, "SI DMA not from start of PIF RAM!");
    process_pif_command(system);

    for (int i = 0; i < 64; i++) {
        byte value = system->mem.pif_ram[i];
        n64_write_byte(system, dram_address + i, value);
    }
    interrupt_raise(INTERRUPT_SI);
}

void dram_to_pif(n64_system_t* system, word dram_address, word pif_address) {
    if ((dram_address & 1) != 0) {
        logfatal("DRAM to PIF on unaligned address");
    }
    unimplemented(pif_address != 0x1FC007C0, "SI DMA not to start of PIF RAM!");
    for (int i = 0; i < 64; i++) {
        system->mem.pif_ram[i] = n64_read_byte(system, dram_address + i);
    }
    process_pif_command(system);
    interrupt_raise(INTERRUPT_SI);
}

void write_word_sireg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_SI_DRAM_ADDR_REG:
            system->mem.si_reg.dram_address = value;
            break;
        case ADDR_SI_PIF_ADDR_RD64B_REG:
            pif_to_dram(system, value, system->mem.si_reg.dram_address & 0x1FFFFFFF);
            break;
        case ADDR_SI_PIF_ADDR_WR64B_REG:
            dram_to_pif(system, system->mem.si_reg.dram_address & 0x1FFFFFFF, value);
            break;
        case ADDR_SI_STATUS_REG:
            interrupt_lower(system, INTERRUPT_SI);
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SI_REGS", value, address);
    }
}

word read_word_sireg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_SI_DRAM_ADDR_REG:
            logfatal("Reading from unimplemented SI register: ADDR_SI_DRAM_ADDR_REG");
        case ADDR_SI_PIF_ADDR_RD64B_REG:
            logfatal("Reading from unimplemented SI register: ADDR_SI_PIF_ADDR_RD64B_REG");
        case ADDR_SI_PIF_ADDR_WR64B_REG:
            logfatal("Reading from unimplemented SI register: ADDR_SI_PIF_ADDR_WR64B_REG");
        case ADDR_SI_STATUS_REG: {
            word value = 0;
            value |= (false << 0); // DMA busy
            value |= (false << 1); // IO read busy
            value |= (false << 3); // DMA error
            value |= (system->mi.intr.si << 12); // SI interrupt
            return value;
        }
        default:
            logfatal("Reading from unknown SI register: 0x%08X", address);
    }
}

word read_unused(word address) {
    logwarn("Reading unused value at 0x%08X!", address);
    return 0;
}

INLINE void invalidate_rsp_icache(n64_system_t* system, word address) {
    if (address >= SREGION_SP_IMEM) {
        address -= SREGION_SP_IMEM;
    }
    address -= (address % 4);

    int index = address / 4;

    system->rsp.icache[index].handler = cache_rsp_instruction;
    system->rsp.icache[index].instruction.raw = word_from_byte_array(system->rsp.sp_imem, address);
}

void n64_write_dword(n64_system_t* system, word address, dword value) {
    logdebug("Writing 0x%016lX to [0x%08X]", value, address);
    invalidate_dynarec_page(system->dynarec, address);
    switch (address) {
        case REGION_RDRAM:
            dword_to_byte_array((byte*) &system->mem.rdram, DWORD_ADDRESS(address) - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_DMEM:
            dword_to_byte_array((byte*) &system->rsp.sp_dmem, DWORD_ADDRESS(address - SREGION_SP_DMEM), value);
            break;
        case REGION_SP_IMEM:
            dword_to_byte_array((byte*) &system->rsp.sp_imem, DWORD_ADDRESS(address - SREGION_SP_IMEM), value);
            invalidate_rsp_icache(system, DWORD_ADDRESS(address));
            invalidate_rsp_icache(system, DWORD_ADDRESS(address) + 4);
            break;
        case REGION_SP_UNUSED:
            return;
        case REGION_SP_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_SP_REGS", value, address);
            break;
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address);
        case REGION_DP_SPAN_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address);
        case REGION_MI_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_MI_REGS", value, address);
            break;
        case REGION_VI_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_VI_REGS", value, address);
            break;
        case REGION_AI_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_AI_REGS", value, address);
            break;
        case REGION_PI_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_PI_REGS", value, address);
            break;
        case REGION_RI_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_RI_REGS", value, address);
            break;
        case REGION_SI_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_SI_REGS", value, address);
            break;
        case REGION_UNUSED:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_UNUSED", value, address);
        case REGION_CART_2_1:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_2_1", value, address);
        case REGION_CART_1_1:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
        case REGION_CART_2_2:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_2_2", value, address);
        case REGION_CART_1_2:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
        case REGION_PIF_BOOT:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address);
        case REGION_PIF_RAM:
            dword_to_byte_array(system->mem.pif_ram, address - SREGION_PIF_RAM, htobe64(value));
            logfatal("Writing dword 0x%016lX to address 0x%08X in region: REGION_PIF_RAM", value, address);
            break;
        case REGION_RESERVED:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_RESERVED", value, address);
        case REGION_CART_1_3:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_1_3", value, address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
            break;
        default:
            logfatal("Writing dword 0x%016lX to unknown address: 0x%08X", value, address);
    }
}

dword n64_read_dword(n64_system_t* system, word address) {
    switch (address) {
        case REGION_RDRAM:
            return dword_from_byte_array((byte*) &system->mem.rdram, DWORD_ADDRESS(address) - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(DWORD_ADDRESS(address));
        case REGION_RDRAM_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_DMEM:
            return dword_from_byte_array((byte*) &system->rsp.sp_dmem, DWORD_ADDRESS(address - SREGION_SP_DMEM));
        case REGION_SP_IMEM:
            return dword_from_byte_array((byte*) &system->rsp.sp_imem, DWORD_ADDRESS(address - SREGION_SP_IMEM));
        case REGION_SP_UNUSED:
            return read_unused(address);
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
        case REGION_CART_2_1:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_CART_1_1", address);
        case REGION_CART_2_2:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_CART_2_2", address);
        case REGION_CART_1_2: {
            word index = DWORD_ADDRESS(address) - SREGION_CART_1_2;
            if (index > system->mem.rom.size - 7) { // -7 because we're reading an entire dword
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
            }
            return dword_from_byte_array(system->mem.rom.rom, index);
        }
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


void n64_write_word(n64_system_t* system, word address, word value) {
    logdebug("Writing 0x%08X to [0x%08X]", value, address);
    invalidate_dynarec_page(system->dynarec, WORD_ADDRESS(address));
    switch (address) {
        case REGION_RDRAM:
            word_to_byte_array((byte*) &system->mem.rdram, WORD_ADDRESS(address) - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            write_word_rdramreg(system, address, value);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_DMEM:
            word_to_byte_array((byte*) &system->rsp.sp_dmem, WORD_ADDRESS(address - SREGION_SP_DMEM), value);
            break;
        case REGION_SP_IMEM:
            word_to_byte_array((byte*) &system->rsp.sp_imem, WORD_ADDRESS(address - SREGION_SP_IMEM), value);
            invalidate_rsp_icache(system, WORD_ADDRESS(address));
            break;
        case REGION_SP_UNUSED:
            return;
        case REGION_SP_REGS:
            write_word_spreg(system, address, value);
            break;
        case REGION_DP_COMMAND_REGS:
            write_word_dpcreg(system, address, value);
            break;
        case REGION_DP_SPAN_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address);
        case REGION_MI_REGS:
            write_word_mireg(system, address, value);
            break;
        case REGION_VI_REGS:
            write_word_vireg(system, address, value);
            break;
        case REGION_AI_REGS:
            write_word_aireg(system, address, value);
            break;
        case REGION_PI_REGS:
            write_word_pireg(system, address, value);
            break;
        case REGION_RI_REGS:
            write_word_rireg(system, address, value);
            break;
        case REGION_SI_REGS:
            write_word_sireg(system, address, value);
            break;
        case REGION_UNUSED:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_UNUSED", value, address);
        case REGION_CART_2_1:
            sram_write_word(system, address - SREGION_CART_2_1, value);
            return;
        case REGION_CART_1_1:
            logwarn("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
            return;
        case REGION_CART_2_2:
            sram_write_word(system, address - SREGION_CART_2_2, value);
            return;
        case REGION_CART_1_2:
            logwarn("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
            return;
        case REGION_PIF_BOOT:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address);
        case REGION_PIF_RAM:
            word_to_byte_array(system->mem.pif_ram, address - SREGION_PIF_RAM, htobe32(value));
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

INLINE word _n64_read_word(word address) {
    switch (address) {
        case REGION_RDRAM:
            return word_from_byte_array((byte*) &global_system->mem.rdram, WORD_ADDRESS(address) - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(address);
        case REGION_RDRAM_REGS:
            return read_word_rdramreg(global_system, address);
        case REGION_SP_DMEM:
            return word_from_byte_array((byte*) &global_system->rsp.sp_dmem, WORD_ADDRESS(address - SREGION_SP_DMEM));
        case REGION_SP_IMEM:
            return word_from_byte_array((byte*) &global_system->rsp.sp_imem, WORD_ADDRESS(address - SREGION_SP_IMEM));
        case REGION_SP_UNUSED:
            return read_unused(address);
        case REGION_SP_REGS:
            return read_word_spreg(global_system, address);
        case REGION_DP_COMMAND_REGS:
            return read_word_dpcreg(global_system, address);
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
        case REGION_DP_SPAN_REGS:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address);
        case REGION_MI_REGS:
            return read_word_mireg(global_system, address);
        case REGION_VI_REGS:
            return read_word_vireg(global_system, address);
        case REGION_AI_REGS:
            return read_word_aireg(global_system, address);
        case REGION_PI_REGS:
            return read_word_pireg(global_system, address);
        case REGION_RI_REGS:
            return read_word_rireg(global_system, address);
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_RI_REGS", address);
        case REGION_SI_REGS:
            return read_word_sireg(global_system, address);
        case REGION_UNUSED:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_UNUSED", address);
        case REGION_CART_2_1:
            return sram_read_word(global_system, address - SREGION_CART_2_1);
        case REGION_CART_1_1:
            logwarn("Reading word from address 0x%08X in unsupported region: REGION_CART_1_1 - This is the N64DD, returning zero because it is not emulated", address);
            return 0;
        case REGION_CART_2_2:
            return sram_read_word(global_system, address - SREGION_CART_2_2);
        case REGION_CART_1_2: {
            word index = WORD_ADDRESS(address) - SREGION_CART_1_2;
            if (index > global_system->mem.rom.size - 3) { // -3 because we're reading an entire word
                logwarn("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
                return 0;
            } else {
                return word_from_byte_array(global_system->mem.rom.rom, index);
            }
        }
        case REGION_PIF_BOOT: {
            if (global_system->mem.rom.pif_rom == NULL) {
                logfatal("Tried to read from PIF ROM, but PIF ROM not loaded!\n");
            } else {
                word index = address - SREGION_PIF_BOOT;
                if (index > global_system->mem.rom.size - 3) { // -3 because we're reading an entire word
                    logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the PIF ROM!", address, index, index);
                } else {
                    return be32toh(word_from_byte_array(global_system->mem.rom.pif_rom, index));
                }
            }
        }
        case REGION_PIF_RAM: {
            if (address == 0x1FC007FC && global_system->mem.pif_ram[63] == 0x30) {
                // TODO Hack to get PIF ROM booting
                // TODO I'm pretty sure this is where the CIC is also supposed to write the checksum out.
                logwarn("Applying hack to get the PIF rom to boot, if you see this message in the middle of a game, something's wrong!");
                global_system->mem.pif_ram[63] = 0x80;
            }
            return be32toh(word_from_byte_array(global_system->mem.pif_ram, address - SREGION_PIF_RAM));
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

word n64_read_word(word address) {
    return _n64_read_word(resolve_virtual_address(address, &global_system->cpu.cp0));
}

word n64_read_physical_word(word address) {
    return _n64_read_word(address);
}

void n64_write_half(n64_system_t* system, word address, half value) {
    logdebug("Writing 0x%04X to [0x%08X]", value, address);
    invalidate_dynarec_page(system->dynarec, HALF_ADDRESS(address));
    switch (address) {
        case REGION_RDRAM:
            half_to_byte_array((byte*) &system->mem.rdram, HALF_ADDRESS(address) - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_DMEM:
            half_to_byte_array((byte*) &system->rsp.sp_dmem, HALF_ADDRESS(address - SREGION_SP_DMEM), value);
            break;
        case REGION_SP_IMEM:
            half_to_byte_array((byte*) &system->rsp.sp_imem, HALF_ADDRESS(address - SREGION_SP_IMEM), value);
            invalidate_rsp_icache(system, HALF_ADDRESS(address));
            break;
        case REGION_SP_UNUSED:
            return;
        case REGION_SP_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_SP_REGS", value, address);
            break;
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address);
        case REGION_DP_SPAN_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address);
        case REGION_MI_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_MI_REGS", value, address);
            break;
        case REGION_VI_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_VI_REGS", value, address);
            break;
        case REGION_AI_REGS:
            logwarn("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_AI_REGS", value, address);
            break;
        case REGION_PI_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_PI_REGS", value, address);
            break;
        case REGION_RI_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_RI_REGS", value, address);
            break;
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_RI_REGS", value, address);
        case REGION_SI_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_SI_REGS", value, address);
            break;
        case REGION_UNUSED:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_UNUSED", value, address);
        case REGION_CART_2_1:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_CART_2_1", value, address);
        case REGION_CART_1_1:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
        case REGION_CART_2_2:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_CART_2_2", value, address);
        case REGION_CART_1_2:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
        case REGION_PIF_BOOT:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address);
        case REGION_PIF_RAM:
            half_to_byte_array((byte*) &system->mem.pif_ram, address - SREGION_PIF_RAM, htobe16(value));
            logfatal("Writing half 0x%04X to address 0x%08X in region: REGION_PIF_RAM", value, address);
            break;
        case REGION_RESERVED:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_RESERVED", value, address);
        case REGION_CART_1_3:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_CART_1_3", value, address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
            break;
        default:
            logfatal("Writing half 0x%04X to unknown address: 0x%08X", value, address);
    }
}

half n64_read_half(n64_system_t* system, word address) {
    switch (address) {
        case REGION_RDRAM:
            return half_from_byte_array((byte*) &system->mem.rdram, HALF_ADDRESS(address) - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(address);
        case REGION_RDRAM_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_DMEM:
            return half_from_byte_array((byte*) &system->rsp.sp_dmem, HALF_ADDRESS(address - SREGION_SP_DMEM));
        case REGION_SP_IMEM:
            return half_from_byte_array((byte*) &system->rsp.sp_imem, HALF_ADDRESS(address - SREGION_SP_IMEM));
        case REGION_SP_UNUSED:
            return read_unused(address);
        case REGION_SP_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_SP_REGS", address);
        case REGION_DP_COMMAND_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
        case REGION_DP_SPAN_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address);
        case REGION_MI_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_MI_REGS", address);
        case REGION_VI_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_VI_REGS", address);
        case REGION_AI_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_AI_REGS", address);
        case REGION_PI_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_PI_REGS", address);
        case REGION_RI_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_RI_REGS", address);
        case REGION_SI_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_SI_REGS", address);
        case REGION_UNUSED:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_UNUSED", address);
        case REGION_CART_2_1:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_CART_1_1", address);
        case REGION_CART_2_2:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_CART_2_2", address);
        case REGION_CART_1_2: {
            word index = HALF_ADDRESS(address) - SREGION_CART_1_2;
            if (index > system->mem.rom.size - 1) { // -1 because we're reading an entire half
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
            }
            return half_from_byte_array(system->mem.rom.rom, index);
        }
        case REGION_PIF_BOOT:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_PIF_BOOT", address);
        case REGION_PIF_RAM:
            logfatal("READHALF FROM PIF RAM\n");
            return be16toh(half_from_byte_array(system->mem.pif_ram, address - SREGION_PIF_RAM));
        case REGION_RESERVED:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_RESERVED", address);
        case REGION_CART_1_3:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_CART_1_3", address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
        default:
            logfatal("Reading half from unknown address: 0x%08X", address);
    }
}

void n64_write_byte(n64_system_t* system, word address, byte value) {
    logdebug("Writing 0x%02X to [0x%08X]", value, address);
    invalidate_dynarec_page(system->dynarec, BYTE_ADDRESS(address));
    switch (address) {
        case REGION_RDRAM:
            system->mem.rdram[BYTE_ADDRESS(address)] = value;
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address);
        case REGION_SP_DMEM: {
            system->rsp.sp_dmem[BYTE_ADDRESS(address - SREGION_SP_DMEM)] = value;
            break;
        }
        case REGION_SP_IMEM: {
            system->rsp.sp_imem[BYTE_ADDRESS(address - SREGION_SP_IMEM)] = value;
            invalidate_rsp_icache(system, BYTE_ADDRESS(address));
            break;
        }
        case REGION_SP_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_SP_REGS", value, address);
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address);
        case REGION_DP_SPAN_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address);
        case REGION_MI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_MI_REGS", value, address);
        case REGION_VI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_VI_REGS", value, address);
        case REGION_AI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_AI_REGS", value, address);
        case REGION_PI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_PI_REGS", value, address);
        case REGION_RI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RI_REGS", value, address);
        case REGION_SI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_SI_REGS", value, address);
        case REGION_UNUSED:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_UNUSED", value, address);
        case REGION_CART_2_1:
            sram_write_byte(system, address - SREGION_CART_2_1, value);
            return;
        case REGION_CART_1_1:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
        case REGION_CART_2_2:
            sram_write_byte(system, address - SREGION_CART_2_2, value);
            return;
        case REGION_CART_1_2:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
        case REGION_PIF_BOOT:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address);
        case REGION_PIF_RAM:
            system->mem.pif_ram[address - SREGION_PIF_RAM] = value;
            logfatal("Writing byte to PIF RAM");
            break;
        case REGION_RESERVED:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RESERVED", value, address);
        case REGION_CART_1_3:
            logwarn("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_3", value, address);
            break;
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
        default:
            logfatal("Writing byte 0x%02X to unknown address: 0x%08X", value, address);
    }
}

byte n64_read_byte(n64_system_t* system, word address) {
    switch (address) {
        case REGION_RDRAM:
            return system->mem.rdram[BYTE_ADDRESS(address)];
        case REGION_RDRAM_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_DMEM:
            return system->rsp.sp_dmem[BYTE_ADDRESS(address) - SREGION_SP_DMEM];
        case REGION_SP_IMEM:
            return system->rsp.sp_imem[BYTE_ADDRESS(address) - SREGION_SP_IMEM];
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
            word w = read_word_aireg(system, address & (~3));
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
        case REGION_CART_2_1:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logwarn("Reading word from address 0x%08X in unsupported region: REGION_CART_1_1 - This is the N64DD, returning zero because it is not emulated", address);
            return 0;
        case REGION_CART_2_2:
            return sram_read_byte(system, address - SREGION_CART_2_2);
        case REGION_CART_1_2: {
            word index = BYTE_ADDRESS(address) - SREGION_CART_1_2;
            if (index > system->mem.rom.size) {
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM! (%ld/0x%lX)", address, index, index, system->mem.rom.size, system->mem.rom.size);
            }
            return system->mem.rom.rom[index];
        }
        case REGION_PIF_BOOT: {
            if (global_system->mem.rom.pif_rom == NULL) {
                logfatal("Tried to read from PIF ROM, but PIF ROM not loaded!\n");
            } else {
                word index = address - SREGION_PIF_BOOT;
                if (index > global_system->mem.rom.size) {
                    logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the PIF ROM!", address, index, index);
                } else {
                    return system->mem.rom.pif_rom[index];
                }
            }
        }
        case REGION_PIF_RAM:
            logfatal("Reading from PIF RAM\n");
            return system->mem.pif_ram[address - SREGION_PIF_RAM];
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
