#include "rsp_interface.h"
#include "rsp.h"

typedef union sp_status_write {
    word raw;
    struct {
        unsigned clear_halt:1;
        unsigned set_halt:1;
        unsigned clear_broke:1;
        unsigned clear_intr:1;
        unsigned set_intr:1;
        unsigned clear_sstep:1;
        unsigned set_sstep:1;
        unsigned clear_intr_on_break:1;
        unsigned set_intr_on_break:1;
        unsigned clear_signal_0:1;
        unsigned set_signal_0:1;
        unsigned clear_signal_1:1;
        unsigned set_signal_1:1;
        unsigned clear_signal_2:1;
        unsigned set_signal_2:1;
        unsigned clear_signal_3:1;
        unsigned set_signal_3:1;
        unsigned clear_signal_4:1;
        unsigned set_signal_4:1;
        unsigned clear_signal_5:1;
        unsigned set_signal_5:1;
        unsigned clear_signal_6:1;
        unsigned set_signal_6:1;
        unsigned clear_signal_7:1;
        unsigned set_signal_7:1;
        unsigned:7;
    };
} sp_status_write_t;

ASSERTWORD(sp_status_write_t);

// If both CLEAR and SET are set, don't change anything. Otherwise either clear or set it.
#define CLEAR_SET(VAL, CLEAR, SET) do { if ((CLEAR) && !(SET)) {(VAL) = false; } if ((SET) && !(CLEAR)) { (VAL) = true; } } while(0)

INLINE void set_rsp_pc(half pc) {
    N64RSP.pc = pc >> 2;
    N64RSP.next_pc = N64RSP.pc + 1;
}

void rsp_status_reg_write(word value) {
    sp_status_write_t write;
    write.raw = value;

    CLEAR_SET(N64RSP.status.halt,          write.clear_halt,          write.set_halt);
    if (N64RSP.status.halt) {
        N64RSP.steps = 0;
    }

    CLEAR_SET(N64RSP.status.broke,         write.clear_broke,         false);
    if (write.clear_intr) {
        interrupt_lower(INTERRUPT_SP);
    }
    if (write.set_intr) {
        interrupt_raise(INTERRUPT_SP);
    }
    CLEAR_SET(N64RSP.status.single_step,   write.clear_sstep,         write.set_sstep);
    CLEAR_SET(N64RSP.status.intr_on_break, write.clear_intr_on_break, write.set_intr_on_break);
    CLEAR_SET(N64RSP.status.signal_0,      write.clear_signal_0,      write.set_signal_0);
    CLEAR_SET(N64RSP.status.signal_1,      write.clear_signal_1,      write.set_signal_1);
    CLEAR_SET(N64RSP.status.signal_2,      write.clear_signal_2,      write.set_signal_2);
    CLEAR_SET(N64RSP.status.signal_3,      write.clear_signal_3,      write.set_signal_3);
    CLEAR_SET(N64RSP.status.signal_4,      write.clear_signal_4,      write.set_signal_4);
    CLEAR_SET(N64RSP.status.signal_5,      write.clear_signal_5,      write.set_signal_5);
    CLEAR_SET(N64RSP.status.signal_6,      write.clear_signal_6,      write.set_signal_6);
    CLEAR_SET(N64RSP.status.signal_7,      write.clear_signal_7,      write.set_signal_7);
}

word read_word_spreg(word address) {
    switch (address) {
        case ADDR_SP_MEM_ADDR_REG:
            return N64RSP.io.mem_addr.raw;
        case ADDR_SP_DRAM_ADDR_REG:
            return N64RSP.io.dram_addr.raw;
        case ADDR_SP_RD_LEN_REG:
        case ADDR_SP_WR_LEN_REG:
            return N64RSP.io.dma.raw;
        case ADDR_SP_PC_REG:
            return (N64RSP.pc << 2) & 0xFFF;
        case ADDR_SP_STATUS_REG:
            return N64RSP.status.raw;
        case ADDR_SP_DMA_BUSY_REG:
            return 0; // DMA not busy, since it's instant.
        case ADDR_SP_SEMAPHORE_REG:
            return N64RSP.semaphore_held;
        default:
            logfatal("Reading word from unknown/unsupported address 0x%08X in region: REGION_SP_REGS", address);
    }
}

void write_word_spreg(word address, word value) {
    switch (address) {
        case ADDR_SP_MEM_ADDR_REG:
            N64RSP.io.shadow_mem_addr.raw = value;
            break;
        case ADDR_SP_DRAM_ADDR_REG:
            N64RSP.io.shadow_dmem_addr.raw = value;
            break;
        case ADDR_SP_RD_LEN_REG: {
            N64RSP.io.dma.raw = value;
            rsp_dma_read();
            break;
        }
        case ADDR_SP_WR_LEN_REG: {
            N64RSP.io.dma.raw = value;
            rsp_dma_write();
            break;
        }
        case ADDR_SP_STATUS_REG:
            rsp_status_reg_write(value);
            break;
        case ADDR_SP_DMA_FULL_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_DMA_FULL_REG");
        case ADDR_SP_DMA_BUSY_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_DMA_BUSY_REG");
        case ADDR_SP_SEMAPHORE_REG:
            if (value == 0) {
                rsp_release_semaphore();
            } else {
                logfatal("Wrote non-zero value 0x%08X to SP reg ADDR_SP_SEMAPHORE_REG", value);
            }
        case ADDR_SP_PC_REG:
            set_rsp_pc(value);
            break;
        case ADDR_SP_IBIST_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_IBIST_REG");
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SP_REGS", value, address);
    }
}

