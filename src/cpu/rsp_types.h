#ifndef N64_RSP_TYPES_H
#define N64_RSP_TYPES_H

#include <stdbool.h>
#include <assert.h>
#include "../common/util.h"

typedef union vu_reg {
    // Used by instructions
    byte bytes[16];
    half elements[8];
    qword single;
    // Only used for loading
    word words[4];
} vu_reg_t;

static_assert(sizeof(vu_reg_t) == 16, "vu_reg_t incorrect size!");

typedef union rsp_types {
    word raw;
    struct {
        bool halt:1;
        bool broke:1;
        bool dma_busy:1;
        bool dma_full:1;
        bool io_full:1;
        bool single_step:1;
        bool intr_on_break:1;
        bool signal_0:1;
        bool signal_1:1;
        bool signal_2:1;
        bool signal_3:1;
        bool signal_4:1;
        bool signal_5:1;
        bool signal_6:1;
        bool signal_7:1;
        unsigned:17;
    };
} rsp_status_t;

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

    vu_reg_t vu_regs[32];

    bool semaphore_held;

    byte (*read_byte)(word);
    void (*write_byte)(word, byte);

    byte (*read_physical_byte)(word);
    void (*write_physical_byte)(word, byte);

    half (*read_half)(word);
    void (*write_half)(word, half);

    word (*read_word)(word);
    void (*write_word)(word, word);

    //dword (*read_dword)(word);
    //void (*write_dword)(word, dword);
} rsp_t;

#endif //N64_RSP_TYPES_H
