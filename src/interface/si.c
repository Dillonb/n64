#include <util.h>
#include <log.h>
#include <mem/pif.h>
#include <mem/mem_util.h>
#include <system/scheduler.h>
#include "si.h"

#define SI_DMA_DELAY (65536 * 2)

void pif_to_dram(word pif_address, word dram_address) {
    if ((dram_address & 1) != 0) {
        logfatal("PIF to DRAM on unaligned address");
    }
    process_pif_command();

    for (int i = 0; i < 64; i++) {
        byte value = n64sys.mem.pif_ram[i];
        RDRAM_BYTE(dram_address + i) = value;
    }
}

void dram_to_pif(word dram_address, word pif_address) {
    if ((dram_address & 1) != 0) {
        logfatal("DRAM to PIF on unaligned address");
    }
    for (int i = 0; i < 64; i++) {
        n64sys.mem.pif_ram[i] = RDRAM_BYTE(dram_address + i);
    }
    process_pif_command();
}

void on_si_dma_complete() {
    n64sys.si.dma_busy = false;

    if (n64sys.si.dma_to_dram) {
        pif_to_dram(0x1FC007C0, n64sys.mem.si_reg.dram_address & 0x1FFFFFFF);
    } else {
        dram_to_pif(n64sys.mem.si_reg.dram_address & 0x1FFFFFFF, 0x1FC007C0);
    }

    interrupt_raise(INTERRUPT_SI);
}

void write_word_sireg(word address, word value) {
    switch (address) {
        case ADDR_SI_DRAM_ADDR_REG:
            n64sys.mem.si_reg.dram_address = value;
            break;
        case ADDR_SI_PIF_ADDR_RD64B_REG:
            n64sys.si.dma_to_dram = true;
            n64sys.si.dma_busy = true;
            value &= 0x1FFFFFFF;
            n64sys.mem.si_reg.pif_address = value;
            unimplemented(value != 0x1FC007C0, "SI DMA not from start of PIF RAM!");
            scheduler_enqueue_relative(SI_DMA_DELAY, SCHEDULER_SI_DMA_COMPLETE);
            break;
        case ADDR_SI_PIF_ADDR_WR64B_REG:
            n64sys.si.dma_to_dram = false;
            n64sys.si.dma_busy = true;
            value &= 0x1FFFFFFF;
            n64sys.mem.si_reg.pif_address = value;
            unimplemented(value != 0x1FC007C0, "SI DMA not to start of PIF RAM!");
            scheduler_enqueue_relative(SI_DMA_DELAY, SCHEDULER_SI_DMA_COMPLETE);
            break;
        case ADDR_SI_STATUS_REG:
            interrupt_lower(INTERRUPT_SI);
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SI_REGS", value, address);
    }
}

word read_word_sireg(word address) {
    switch (address) {
        case ADDR_SI_DRAM_ADDR_REG:
            return n64sys.mem.si_reg.dram_address;
        case ADDR_SI_PIF_ADDR_RD64B_REG:
            return n64sys.mem.si_reg.pif_address;
        case ADDR_SI_PIF_ADDR_WR64B_REG:
            return n64sys.mem.si_reg.pif_address;
        case ADDR_SI_STATUS_REG: {
            word value = 0;
            value |= (n64sys.si.dma_busy << 0); // DMA busy
            value |= (false << 1); // IO read busy
            value |= (false << 3); // DMA error
            value |= (n64sys.mi.intr.si << 12); // SI interrupt
            return value;
        }
        case ADDR_SI_UNKNOWN_REG_08:
            logwarn("Reading from unknown SI register: 0x04800008 - returning 0xFFFFFFFF.");
            return 0xFFFFFFFF; // Value is not hardware accurate. Need more research.
        case ADDR_SI_UNKNOWN_REG_0C:
            logwarn("Reading from unknown SI register: 0x0480000C - returning 0.");
            return 0; // Reasonably sure this one is correct.
        case ADDR_SI_UNKNOWN_REG_14:
            logwarn("Reading from unknown SI register: 0x04800014 - returning 0xFFFFFFFF.");
            return 0xFFFFFFFF; // Value is not hardware accurate. Need more research.
        case ADDR_SI_UNKNOWN_REG_1C:
            logwarn("Reading from unknown SI register: 0x0480001C - returning 0xFFFFFFFF.");
            return 0xFFFFFFFF; // Value is not hardware accurate. Need more research.
        default:
            logfatal("Reading from unknown SI register: 0x%08X", address);
    }
}
