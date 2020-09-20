#include "n64bus.h"

#include <interface/vi.h>
#include <interface/ai.h>
#include <cpu/rsp_interface.h>
#include <rdp/rdp.h>

#include "dma.h"
#include "addresses.h"
#include "pif.h"
#include "mem_util.h"

word get_vpn(word address, word page_mask_raw) {
    word tmp = page_mask_raw | 0x1FFF;
    word vpn = address & ~tmp;

    return vpn;
}

bool tlb_probe(word vaddr, word* paddr, int* entry_number, cp0_t* cp0) {
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
            pfn = entry.entry_lo0.entry;
        } else {
            if (!(entry.entry_lo1.valid)) {
                continue;
            }
            pfn = entry.entry_lo1.entry;
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
            logfatal("Reading word from unsupported PI register: PI_CART_ADDR_REG");
        case ADDR_PI_RD_LEN_REG:
            logfatal("Reading word from unsupported PI register: PI_RD_LEN_REG");
        case ADDR_PI_WR_LEN_REG:
            logfatal("Reading word from unsupported PI register: PI_WR_LEN_REG");
        case ADDR_PI_STATUS_REG: {
            word value = 0;
            value |= is_dma_active();
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
            system->mem.pi_reg[PI_DRAM_ADDR_REG] = value;
            //system->mem.pi_reg[PI_DRAM_ADDR_REG] = value & ~7;
            break;
        case ADDR_PI_CART_ADDR_REG:
            system->mem.pi_reg[PI_CART_ADDR_REG] = value;
            //system->mem.pi_reg[PI_CART_ADDR_REG] = value & ~1;
            break;
        case ADDR_PI_RD_LEN_REG: {
            word length = ((value & 0x00FFFFFF) | 7) + 1;
            system->mem.pi_reg[PI_RD_LEN_REG] = length;
            run_dma(system, system->mem.pi_reg[PI_DRAM_ADDR_REG], system->mem.pi_reg[PI_CART_ADDR_REG], length, "DRAM to CART");
            interrupt_raise(system, INTERRUPT_PI);
            break;
        }
        case ADDR_PI_WR_LEN_REG: {
            word length = ((value & 0x00FFFFFF) | 7) + 1;
            system->mem.pi_reg[PI_WR_LEN_REG] = length;
            run_dma(system, system->mem.pi_reg[PI_CART_ADDR_REG], system->mem.pi_reg[PI_DRAM_ADDR_REG], length, "CART to DRAM");
            interrupt_raise(system, INTERRUPT_PI);
            break;
        }
        case ADDR_PI_STATUS_REG: {
            if (value & 0b01) {
                logfatal("TODO: Set PI error to 0");
            }
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

    return 0;
}

void write_word_rireg(n64_system_t* system, word address, word value) {
    if (address % 4 != 0) {
        logfatal("Writing to RI register at non-word-aligned address 0x%08X", address);
    }

    if (address < ADDR_RI_FIRST || address > ADDR_RI_LAST) {
        logfatal("In RI write handler with out of bounds address 0x%08X", address);
    }

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
            return 0x01010101;
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
    interrupt_raise(system, INTERRUPT_SI);
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
    interrupt_raise(system, INTERRUPT_SI);
}

void write_word_sireg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_SI_DRAM_ADDR_REG:
            system->mem.si_reg.dram_address = value;
            break;
        case ADDR_SI_PIF_ADDR_RD64B_REG:
            pif_to_dram(system, value, system->mem.si_reg.dram_address);
            break;
        case ADDR_SI_PIF_ADDR_WR64B_REG:
            dram_to_pif(system, system->mem.si_reg.dram_address, value);
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

    system->rsp.icache[index].type = MIPS_UNKNOWN;
    system->rsp.icache[index].instruction.raw = 0;
}

void n64_write_dword(n64_system_t* system, word address, dword value) {
    logdebug("Writing 0x%016lX to [0x%08X]", value, address);
    switch (address) {
        case REGION_RDRAM:
            dword_to_byte_array((byte*) &system->mem.rdram, address - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_DMEM:
            dword_to_byte_array((byte*) &system->mem.sp_dmem, address - SREGION_SP_DMEM, value);
            break;
        case REGION_SP_IMEM:
            dword_to_byte_array((byte*) &system->mem.sp_imem, address - SREGION_SP_IMEM, value);
            invalidate_rsp_icache(system, address);
            invalidate_rsp_icache(system, address + 4);
            break;
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_SP_IMEM", value, address);
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
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_RI_REGS", value, address);
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
            dword_to_byte_array(system->mem.pif_ram, address - SREGION_PIF_RAM, value);
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
            return dword_from_byte_array((byte*) &system->mem.rdram, address - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(address);
        case REGION_RDRAM_REGS:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_DMEM:
            return dword_from_byte_array((byte*) &system->mem.sp_dmem, address - SREGION_SP_DMEM);
        case REGION_SP_IMEM:
            return dword_from_byte_array((byte*) &system->mem.sp_imem, address - SREGION_SP_IMEM);
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
            word index = address - SREGION_CART_1_2;
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
    switch (address) {
        case REGION_RDRAM:
            word_to_byte_array((byte*) &system->mem.rdram, address - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            write_word_rdramreg(system, address, value);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_DMEM:
            word_to_byte_array((byte*) &system->mem.sp_dmem, address - SREGION_SP_DMEM, value);
            break;
        case REGION_SP_IMEM:
            word_to_byte_array((byte*) &system->mem.sp_imem, address - SREGION_SP_IMEM, value);
            invalidate_rsp_icache(system, address);
            break;
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SP_IMEM", value, address);
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
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_RI_REGS", value, address);
        case REGION_SI_REGS:
            write_word_sireg(system, address, value);
            break;
        case REGION_UNUSED:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_UNUSED", value, address);
        case REGION_CART_2_1:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_2_1", value, address);
        case REGION_CART_1_1:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
        case REGION_CART_2_2:
            logwarn("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_2_2", value, address);
            return;
        case REGION_CART_1_2:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
        case REGION_PIF_BOOT:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address);
        case REGION_PIF_RAM:
            word_to_byte_array(system->mem.pif_ram, address - SREGION_PIF_RAM, value);
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

word n64_read_word(n64_system_t* system, word address) {
    switch (address) {
        case REGION_RDRAM:
            return word_from_byte_array((byte*) &system->mem.rdram, address - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(address);
        case REGION_RDRAM_REGS:
            return read_word_rdramreg(system, address);
        case REGION_SP_DMEM:
            return word_from_byte_array((byte*) &system->mem.sp_dmem, address - SREGION_SP_DMEM);
        case REGION_SP_IMEM:
            return word_from_byte_array((byte*) &system->mem.sp_imem, address - SREGION_SP_IMEM);
        case REGION_SP_UNUSED:
            return read_unused(address);
        case REGION_SP_REGS:
            return read_word_spreg(system, address);
        case REGION_DP_COMMAND_REGS:
            return read_word_dpcreg(system, address);
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
        case REGION_DP_SPAN_REGS:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address);
        case REGION_MI_REGS:
            return read_word_mireg(system, address);
        case REGION_VI_REGS:
            return read_word_vireg(system, address);
        case REGION_AI_REGS:
            return read_word_aireg(system, address);
        case REGION_PI_REGS:
            return read_word_pireg(system, address);
        case REGION_RI_REGS:
            return read_word_rireg(system, address);
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_RI_REGS", address);
        case REGION_SI_REGS:
            return read_word_sireg(system, address);
        case REGION_UNUSED:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_UNUSED", address);
        case REGION_CART_2_1:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_CART_1_1", address);
        case REGION_CART_2_2:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_CART_2_2", address);
        case REGION_CART_1_2: {
            word index = address - SREGION_CART_1_2;
            if (index > system->mem.rom.size - 3) { // -3 because we're reading an entire word
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
            }
            return word_from_byte_array(system->mem.rom.rom, index);
        }
        case REGION_PIF_BOOT:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_PIF_BOOT", address);
        case REGION_PIF_RAM:
            return word_from_byte_array(system->mem.pif_ram, address - SREGION_PIF_RAM);
        case REGION_RESERVED:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_RESERVED", address);
        case REGION_CART_1_3:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_CART_1_3", address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
        default:
            logfatal("Reading word from unknown address: 0x%08X", address);
    }
}

void n64_write_half(n64_system_t* system, word address, half value) {
    logdebug("Writing 0x%04X to [0x%08X]", value, address);
    switch (address) {
        case REGION_RDRAM:
            half_to_byte_array((byte*) &system->mem.rdram, address - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_DMEM:
            half_to_byte_array((byte*) &system->mem.sp_dmem, address - SREGION_SP_DMEM, value);
            break;
        case REGION_SP_IMEM:
            half_to_byte_array((byte*) &system->mem.sp_imem, address - SREGION_SP_IMEM, value);
            invalidate_rsp_icache(system, address);
            break;
            logfatal("Writing half 0x%04X to address 0x%08X in unsupported region: REGION_SP_IMEM", value, address);
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
            half_to_byte_array((byte*) &system->mem.pif_ram, address - SREGION_PIF_RAM, value);
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
            return half_from_byte_array((byte*) &system->mem.rdram, address - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(address);
        case REGION_RDRAM_REGS:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_DMEM:
            return half_from_byte_array((byte*) &system->mem.sp_dmem, address - SREGION_SP_DMEM);
        case REGION_SP_IMEM:
            return half_from_byte_array((byte*) &system->mem.sp_imem, address - SREGION_SP_IMEM);
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
            half index = address - SREGION_CART_1_2;
            if (index > system->mem.rom.size - 1) { // -1 because we're reading an entire half
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
            }
            return half_from_byte_array(system->mem.rom.rom, index);
        }
        case REGION_PIF_BOOT:
            logfatal("Reading half from address 0x%08X in unsupported region: REGION_PIF_BOOT", address);
        case REGION_PIF_RAM:
            logfatal("READHALF FROM PIF RAM\n");
            return half_from_byte_array(system->mem.pif_ram, address - SREGION_PIF_RAM);
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
    switch (address) {
        case REGION_RDRAM:
            system->mem.rdram[address] = value;
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address);
        case REGION_SP_DMEM: {
            system->mem.sp_dmem[address - SREGION_SP_DMEM] = value;
            break;
        }
        case REGION_SP_IMEM: {
            system->mem.sp_imem[address - SREGION_SP_IMEM] = value;
            invalidate_rsp_icache(system, address);
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
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_2_1", value, address);
        case REGION_CART_1_1:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
        case REGION_CART_2_2:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_2_2", value, address);
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
            return system->mem.rdram[address];
        case REGION_RDRAM_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address);
        case REGION_SP_DMEM:
            return system->mem.sp_dmem[address - SREGION_SP_DMEM];
        case REGION_SP_IMEM:
            return system->mem.sp_imem[address - SREGION_SP_IMEM];
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
        case REGION_AI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_AI_REGS", address);
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
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_1", address);
        case REGION_CART_2_2:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_2_2", address);
        case REGION_CART_1_2: {
            word index = address - SREGION_CART_1_2;
            if (index > system->mem.rom.size) {
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
            }
            return system->mem.rom.rom[index];
        }
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_2", address);
        case REGION_PIF_BOOT:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_PIF_BOOT", address);
        case REGION_PIF_RAM:
            logfatal("Reading from PIF RAM\n");
            return system->mem.pif_ram[address - SREGION_PIF_RAM];
        case REGION_RESERVED:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RESERVED", address);
        case REGION_CART_1_3:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_3", address);
        case REGION_SYSAD_DEVICE:
            logfatal("This is a virtual address!");
        default:
            logfatal("Reading byte from unknown address: 0x%08X", address);
    }
}
