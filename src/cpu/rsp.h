#ifndef N64_RSP_H
#define N64_RSP_H

#include <stdbool.h>
#include "../common/util.h"
#include "../common/log.h"

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
    word pc;
    //dword mult_hi;
    //dword mult_lo;

    // Branch delay
    bool branch;
    int branch_delay;
    word branch_pc;

    byte (*read_byte)(word);
    void (*write_byte)(word, byte);

    half (*read_half)(word);
    void (*write_half)(word, half);

    word (*read_word)(word);
    void (*write_word)(word, word);

    dword (*read_dword)(word);
    void (*write_dword)(word, dword);
} rsp_t;

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
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_DMA_CACHE")
        case RSP_CP0_DMA_DRAM:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_DMA_DRAM")
        case RSP_CP0_DMA_READ_LENGTH:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_DMA_READ_LENGTH")
        case RSP_CP0_DMA_WRITE_LENGTH:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_DMA_WRITE_LENGTH")
        case RSP_CP0_SP_STATUS:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_SP_STATUS")
        case RSP_CP0_DMA_FULL:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_DMA_FULL")
        case RSP_CP0_DMA_BUSY:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_DMA_BUSY")
        case RSP_CP0_DMA_RESERVED:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_DMA_RESERVED")
        case RSP_CP0_CMD_START:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_CMD_START")
        case RSP_CP0_CMD_END:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_CMD_END")
        case RSP_CP0_CMD_CURRENT:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_CMD_CURRENT")
        case RSP_CP0_CMD_STATUS:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_CMD_STATUS")
        case RSP_CP0_CMD_CLOCK:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_CMD_CLOCK")
        case RSP_CP0_CMD_BUSY:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_CMD_BUSY")
        case RSP_CP0_CMD_PIPE_BUSY:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_CMD_PIPE_BUSY")
        case RSP_CP0_CMD_TMEM_BUSY:
            logfatal("Read from unknown RSP CP0 register: RSP_CP0_CMD_TMEM_BUSY")
        default:
            logfatal("Unsupported RSP CP0 %d read", r)
    }
}

INLINE void set_rsp_cp0_register(rsp_t* rsp, byte r, word value) {
    switch (r) {
        case RSP_CP0_DMA_CACHE:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_DMA_CACHE")
        case RSP_CP0_DMA_DRAM:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_DMA_DRAM")
        case RSP_CP0_DMA_READ_LENGTH:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_DMA_READ_LENGTH")
        case RSP_CP0_DMA_WRITE_LENGTH:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_DMA_WRITE_LENGTH")
        case RSP_CP0_SP_STATUS:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_SP_STATUS")
        case RSP_CP0_DMA_FULL:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_DMA_FULL")
        case RSP_CP0_DMA_BUSY:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_DMA_BUSY")
        case RSP_CP0_DMA_RESERVED:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_DMA_RESERVED")
        case RSP_CP0_CMD_START:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_CMD_START")
        case RSP_CP0_CMD_END:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_CMD_END")
        case RSP_CP0_CMD_CURRENT:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_CMD_CURRENT")
        case RSP_CP0_CMD_STATUS:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_CMD_STATUS")
        case RSP_CP0_CMD_CLOCK:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_CMD_CLOCK")
        case RSP_CP0_CMD_BUSY:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_CMD_BUSY")
        case RSP_CP0_CMD_PIPE_BUSY:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_CMD_PIPE_BUSY")
        case RSP_CP0_CMD_TMEM_BUSY:
            logfatal("Write to unknown RSP CP0 register: RSP_CP0_CMD_TMEM_BUSY")
        default:
            logfatal("Unsupported RSP CP0 %d read", r)
    }
}

void rsp_step(rsp_t* rsp);

#endif //N64_RSP_H
