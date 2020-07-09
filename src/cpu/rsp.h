#ifndef N64_RSP_H
#define N64_RSP_H

#include <stdbool.h>
#include "../common/util.h"
#include "../common/log.h"
#include "rsp_types.h"
#include "../mem/addresses.h"
#include "../system/n64system.h"

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

bool rsp_acquire_semaphore(n64_system_t* system);

INLINE word get_rsp_cp0_register(n64_system_t* system, byte r) {
    switch (r) {
        case RSP_CP0_DMA_CACHE:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_CACHE", r)
        case RSP_CP0_DMA_DRAM:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_DRAM", r)
        case RSP_CP0_DMA_READ_LENGTH:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_READ_LENGTH", r)
        case RSP_CP0_DMA_WRITE_LENGTH:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_WRITE_LENGTH", r)
        case RSP_CP0_SP_STATUS: return system->rsp.status.raw;
        case RSP_CP0_DMA_FULL:  return system->rsp.status.dma_full;
        case RSP_CP0_DMA_BUSY:  return system->rsp.status.dma_busy;
        case RSP_CP0_DMA_RESERVED: return rsp_acquire_semaphore(system);
        case RSP_CP0_CMD_START:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_START", r)
        case RSP_CP0_CMD_END:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_END", r)
        case RSP_CP0_CMD_CURRENT:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_CURRENT", r)
        case RSP_CP0_CMD_STATUS: return system->dpc.status.raw;
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

INLINE void set_rsp_cp0_register(n64_system_t* system, byte r, word value) {
    switch (r) {
        case RSP_CP0_DMA_CACHE: system->rsp.io.mem_addr.raw = value; break;
        case RSP_CP0_DMA_DRAM:  system->rsp.io.dram_addr.raw = value; break;
        case RSP_CP0_DMA_READ_LENGTH:
            system->rsp.io.dma_read.raw = value;
            rsp_dma_read(&system->rsp);
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
                system->rsp.semaphore_held = 0;
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

void rsp_step(n64_system_t* system);

#endif //N64_RSP_H
