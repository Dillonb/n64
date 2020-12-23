#include "rsp_interface.h"
#include "rsp.h"

typedef union sp_status_write {
    word raw;
    struct {
        bool clear_halt:1;
        bool set_halt:1;
        bool clear_broke:1;
        bool clear_intr:1;
        bool set_intr:1;
        bool clear_sstep:1;
        bool set_sstep:1;
        bool clear_intr_on_break:1;
        bool set_intr_on_break:1;
        bool clear_signal_0:1;
        bool set_signal_0:1;
        bool clear_signal_1:1;
        bool set_signal_1:1;
        bool clear_signal_2:1;
        bool set_signal_2:1;
        bool clear_signal_3:1;
        bool set_signal_3:1;
        bool clear_signal_4:1;
        bool set_signal_4:1;
        bool clear_signal_5:1;
        bool set_signal_5:1;
        bool clear_signal_6:1;
        bool set_signal_6:1;
        bool clear_signal_7:1;
        bool set_signal_7:1;
        unsigned:7;
    };
} sp_status_write_t;

#define CLEAR_SET(VAL, CLEAR, SET) if (CLEAR) {VAL = false; } if (SET) { VAL = true; }

void rsp_status_reg_write(n64_system_t* system, word value) {
    sp_status_write_t write;
    write.raw = value;

    CLEAR_SET(system->rsp.status.halt,          write.clear_halt,          write.set_halt)
    if (system->rsp.status.halt) {
        system->rsp.steps = 0;
    }

    CLEAR_SET(system->rsp.status.broke,         write.clear_broke,         false)
    if (write.clear_intr) {
        interrupt_lower(system, INTERRUPT_SP);
    }
    if (write.set_intr) {
        interrupt_raise(INTERRUPT_SP);
    }
    CLEAR_SET(system->rsp.status.single_step,   write.clear_sstep,         write.set_sstep)
    CLEAR_SET(system->rsp.status.intr_on_break, write.clear_intr_on_break, write.set_intr_on_break)
    CLEAR_SET(system->rsp.status.signal_0,      write.clear_signal_0,      write.set_signal_0)
    CLEAR_SET(system->rsp.status.signal_1,      write.clear_signal_1,      write.set_signal_1)
    CLEAR_SET(system->rsp.status.signal_2,      write.clear_signal_2,      write.set_signal_2)
    CLEAR_SET(system->rsp.status.signal_3,      write.clear_signal_3,      write.set_signal_3)
    CLEAR_SET(system->rsp.status.signal_4,      write.clear_signal_4,      write.set_signal_4)
    CLEAR_SET(system->rsp.status.signal_5,      write.clear_signal_5,      write.set_signal_5)
    CLEAR_SET(system->rsp.status.signal_6,      write.clear_signal_6,      write.set_signal_6)
    CLEAR_SET(system->rsp.status.signal_7,      write.clear_signal_7,      write.set_signal_7)
}

word read_word_spreg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_SP_MEM_ADDR_REG:
            return system->rsp.io.mem_addr.raw;
        case ADDR_SP_DRAM_ADDR_REG:
            return system->rsp.io.dram_addr.raw;
        case ADDR_SP_RD_LEN_REG:
            return system->rsp.io.dma.raw;
        case ADDR_SP_WR_LEN_REG:
            return system->rsp.io.dma.raw;
        case ADDR_SP_PC_REG:
            return system->rsp.pc;
        case ADDR_SP_STATUS_REG:
            return system->rsp.status.raw;
        case ADDR_SP_DMA_BUSY_REG:
            return 0; // DMA not busy, since it's instant.
        default:
            logfatal("Reading word from unknown/unsupported address 0x%08X in region: REGION_SP_REGS", address);
    }
}

void write_word_spreg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_SP_MEM_ADDR_REG:
            system->rsp.io.shadow_mem_addr.raw = value;
            break;
        case ADDR_SP_DRAM_ADDR_REG:
            system->rsp.io.shadow_dmem_addr.raw = value;
            break;
        case ADDR_SP_RD_LEN_REG: {
            system->rsp.io.dma.raw = value;
            rsp_dma_read(&system->rsp);
            break;
        }
        case ADDR_SP_WR_LEN_REG: {
            system->rsp.io.dma.raw = value;
            rsp_dma_write(&system->rsp);
            break;
        }
        case ADDR_SP_STATUS_REG:
            rsp_status_reg_write(system, value);
            break;
        case ADDR_SP_DMA_FULL_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_DMA_FULL_REG");
        case ADDR_SP_DMA_BUSY_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_DMA_BUSY_REG");
        case ADDR_SP_SEMAPHORE_REG:
            if (value == 0) {
                rsp_release_semaphore(system);
            } else {
                logfatal("Wrote non-zero value 0x%08X to SP reg ADDR_SP_SEMAPHORE_REG", value);
            }
        case ADDR_SP_PC_REG:
            set_rsp_pc(&system->rsp, value);
            break;
        case ADDR_SP_IBIST_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_IBIST_REG");
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SP_REGS", value, address);
    }
}

