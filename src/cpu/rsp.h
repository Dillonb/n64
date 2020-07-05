#ifndef N64_RSP_H
#define N64_RSP_H

#include <stdbool.h>
#include "../common/util.h"
#include "../common/log.h"

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

void rsp_step(rsp_t* rsp);

#endif //N64_RSP_H
