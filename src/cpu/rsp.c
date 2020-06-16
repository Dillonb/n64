#include "rsp.h"
#include "../common/log.h"

#define ADDR_SP_MEM_ADDR_REG  0x04040000
#define ADDR_SP_DRAM_ADDR_REG 0x04040004
#define ADDR_SP_RD_LEN_REG    0x04040008
#define ADDR_SP_WR_LEN_REG    0x0404000C
#define ADDR_SP_STATUS_REG    0x04040010
#define ADDR_SP_DMA_FULL_REG  0x04040014
#define ADDR_SP_DMA_BUSY_REG  0x04040018
#define ADDR_SP_SEMAPHORE_REG 0x0404001C
#define ADDR_SP_PC_REG        0x04080000
#define ADDR_SP_IBIST_REG     0x04080004

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

word read_word_spreg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_SP_PC_REG:
            return 0;
        default:
            logfatal("Reading word from unknown/unsupported address 0x%08X in region: REGION_SP_REGS", address)
    }
}

#define CLEAR_SET(VAL, CLEAR, SET) if (CLEAR) {VAL = false; } if (SET) { VAL = true; }

INLINE void status_reg_write(n64_system_t* system, word value) {
    sp_status_write_t write;
    write.raw = value;

    CLEAR_SET(system->rsp_status.halt,          write.clear_halt,          write.set_halt)
    CLEAR_SET(system->rsp_status.broke,         write.clear_broke,         false)
    // CLEAR_SET(system->rsp_status.intr,          write.clear_intr,          write.set_intr)
    if (write.clear_intr) {
        logwarn("TODO: Clearing RSP intr?")
    }
    if (write.set_intr) {
        logwarn("TODO: Setting RSP intr?")
    }
    CLEAR_SET(system->rsp_status.single_step,   write.clear_sstep,         write.set_sstep)
    CLEAR_SET(system->rsp_status.intr_on_break, write.clear_intr_on_break, write.set_intr_on_break)
    CLEAR_SET(system->rsp_status.signal_0,      write.clear_signal_0,      write.set_signal_0)
    CLEAR_SET(system->rsp_status.signal_1,      write.clear_signal_1,      write.set_signal_1)
    CLEAR_SET(system->rsp_status.signal_2,      write.clear_signal_2,      write.set_signal_2)
    CLEAR_SET(system->rsp_status.signal_3,      write.clear_signal_3,      write.set_signal_3)
    CLEAR_SET(system->rsp_status.signal_4,      write.clear_signal_4,      write.set_signal_4)
    CLEAR_SET(system->rsp_status.signal_5,      write.clear_signal_5,      write.set_signal_5)
    CLEAR_SET(system->rsp_status.signal_6,      write.clear_signal_6,      write.set_signal_6)
    CLEAR_SET(system->rsp_status.signal_7,      write.clear_signal_7,      write.set_signal_7)

    //logfatal("Write to status reg")
}

void write_word_spreg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_SP_STATUS_REG:
            status_reg_write(system, value);
            printf("Write to SP status\n");
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SP_REGS", value, address)
    }
}
