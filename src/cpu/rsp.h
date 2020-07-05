#ifndef N64_RSP_H
#define N64_RSP_H

#include <stdbool.h>
#include "../common/util.h"
#include "../common/log.h"
#include "rsp_status.h"
#include "../mem/addresses.h"

#define RSP_CP0_DMA_CACHE        0
#define RSP_CP0_DMA_DRAM         1
#define RSP_CP0_DMA_READ_LENGTH  2
#define RSP_CP0_DMA_WRITE_LENGTH 3
#define RSP_CP0_SP_STATUS        4
#define RSP_CP0_DMA_FULL         5
#define RSP_CP0_DMA_BUSY         6
#define RSP_CP0_DMA_RESERVED     7
#define RSP_CP0_CMD_START        8
#define RSP_CP0_CMD_END          9
#define RSP_CP0_CMD_CURRENT      10
#define RSP_CP0_CMD_STATUS       11
#define RSP_CP0_CMD_CLOCK        12
#define RSP_CP0_CMD_BUSY         13
#define RSP_CP0_CMD_PIPE_BUSY    14
#define RSP_CP0_CMD_TMEM_BUSY    15


typedef struct rsp {
    word gpr[32];
    half pc;
    //dword mult_hi;
    //dword mult_lo;

    rsp_status_t status;

    // Branch delay
    bool branch;
    int branch_delay;
    word branch_pc;

    struct {
        union {
            word raw;
            struct {
                unsigned address:12;
                bool imem:1;
                unsigned:19;
            };
        } mem_addr;
        union {
            word raw;
            struct {
                unsigned address:24;
                unsigned:8;
            };
        } dram_addr;
        union {
            struct {
                unsigned length: 12;
                unsigned count: 8;
                unsigned skip: 12;
            };
            word raw;
        } dma_read;
    } io;

    bool semaphore_held;

    //byte (*read_byte)(word);
    //void (*write_byte)(word, byte);

    byte (*read_physical_byte)(word);
    void (*write_physical_byte)(word, byte);

    //half (*read_half)(word);
    //void (*write_half)(word, half);

    word (*read_word)(word);
    //void (*write_word)(word, word);

    //dword (*read_dword)(word);
    //void (*write_dword)(word, dword);
} rsp_t;

INLINE void rsp_dma_read(rsp_t* rsp) {
    word length = (rsp->io.dma_read.length | 7) + 1;
    for (int i = 0; i < rsp->io.dma_read.count + 1; i++) {
        word mem_addr = rsp->io.mem_addr.address + (rsp->io.mem_addr.imem ? SREGION_SP_IMEM : SREGION_SP_DMEM);
        for (int j = 0; j < length; j++) {
            byte val = rsp->read_physical_byte(rsp->io.dram_addr.address + j);
            logtrace("SP DMA: Copying 0x%02X from 0x%08X to 0x%08X", val, rsp->io.dram_addr.address + j, mem_addr + j)
            rsp->write_physical_byte(mem_addr + j, val);
        }

        rsp->io.dram_addr.address += length + rsp->io.dma_read.skip;
        rsp->io.mem_addr.address += length;
    }
}

INLINE void set_rsp_register(rsp_t* rsp, byte r, word value) {
    logtrace("Setting RSP r%d to [0x%08X]", r, value)
    if (r != 0) {
        if (r < 64) {
            rsp->gpr[r] = value;
        } else {
            logfatal("Write to invalid RSP register: %d", r)
        }
    }
}

INLINE word get_rsp_register(rsp_t* rsp, byte r) {
    if (r < 64) {
        word value = rsp->gpr[r];
        logtrace("Reading RSP r%d: 0x%08X", r, value)
        return value;
    } else {
        logfatal("Attempted to read invalid RSP register: %d", r)
    }
}

INLINE word get_rsp_cp0_register(rsp_t* rsp, byte r) {
    switch (r) {
        case RSP_CP0_DMA_CACHE:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_CACHE", r)
        case RSP_CP0_DMA_DRAM:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_DRAM", r)
        case RSP_CP0_DMA_READ_LENGTH:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_READ_LENGTH", r)
        case RSP_CP0_DMA_WRITE_LENGTH:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_WRITE_LENGTH", r)
        case RSP_CP0_SP_STATUS: return rsp->status.raw;
        case RSP_CP0_DMA_FULL:  return rsp->status.dma_full;
        case RSP_CP0_DMA_BUSY:  return rsp->status.dma_busy;
        case RSP_CP0_DMA_RESERVED:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_RESERVED", r)
        case RSP_CP0_CMD_START:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_START", r)
        case RSP_CP0_CMD_END:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_END", r)
        case RSP_CP0_CMD_CURRENT:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_CURRENT", r)
        case RSP_CP0_CMD_STATUS:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_STATUS", r)
        case RSP_CP0_CMD_CLOCK:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_CLOCK", r)
        case RSP_CP0_CMD_BUSY:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_BUSY", r)
        case RSP_CP0_CMD_PIPE_BUSY:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_PIPE_BUSY", r)
        case RSP_CP0_CMD_TMEM_BUSY:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_TMEM_BUSY", r)
        default:
            logfatal("Unsupported RSP CP0 $c%d read", r)
    }
}

INLINE void set_rsp_cp0_register(rsp_t* rsp, byte r, word value) {
    switch (r) {
        case RSP_CP0_DMA_CACHE: rsp->io.mem_addr.raw = value; break;
        case RSP_CP0_DMA_DRAM:  rsp->io.dram_addr.raw = value; break;
        case RSP_CP0_DMA_READ_LENGTH:
            rsp->io.dma_read.raw = value;
            rsp_dma_read(rsp);
            break;
        case RSP_CP0_DMA_WRITE_LENGTH:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_DMA_WRITE_LENGTH", r)
        case RSP_CP0_SP_STATUS:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_SP_STATUS", r)
        case RSP_CP0_DMA_FULL:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_DMA_FULL", r)
        case RSP_CP0_DMA_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_DMA_BUSY", r)
        case RSP_CP0_DMA_RESERVED: {
            if (value == 0) {
                rsp->semaphore_held = 0;
            } else {
                logfatal("Wrote non-zero value 0x%08X to $c7 RSP_CP0_DMA_RESERVED", value)
            }
            break;
        }
        case RSP_CP0_CMD_START:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_START", r)
        case RSP_CP0_CMD_END:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_END", r)
        case RSP_CP0_CMD_CURRENT:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_CURRENT", r)
        case RSP_CP0_CMD_STATUS:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_STATUS", r)
        case RSP_CP0_CMD_CLOCK:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_CLOCK", r)
        case RSP_CP0_CMD_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_BUSY", r)
        case RSP_CP0_CMD_PIPE_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_PIPE_BUSY", r)
        case RSP_CP0_CMD_TMEM_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_TMEM_BUSY", r)
        default:
            logfatal("Unsupported RSP CP0 $c%d written to", r)
    }
}

void rsp_step(rsp_t* rsp);

#endif //N64_RSP_H
