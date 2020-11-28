#ifndef N64_RSP_TYPES_H
#define N64_RSP_TYPES_H

#include <stdbool.h>
#include <assert.h>
#ifdef N64_USE_SIMD
#include <emmintrin.h>
#endif
#include <util.h>
#include "mips_instruction_decode.h"

#define vecr __m128i

typedef union vu_reg {
    // Used by instructions
    byte bytes[16];
    shalf signed_elements[8];
    half elements[8];
#ifdef N64_USE_SIMD
    vecr single;
#endif
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

typedef struct rsp rsp_t;

typedef void(*rspinstr_handler_t)(rsp_t*, mips_instruction_t);

typedef struct rsp_icache_entry {
    mips_instruction_t instruction;
    rspinstr_handler_t handler;
} rsp_icache_entry_t;

typedef struct rsp {
    word gpr[32];
    half pc;
    half next_pc;
    //dword mult_hi;
    //dword mult_lo;

    vecr zero;

    rsp_status_t status;

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
        union {
            struct {
                unsigned length: 12;
                unsigned count: 8;
                unsigned skip: 12;
            };
            word raw;
        } dma_write;
    } io;

    rsp_icache_entry_t icache[0x1000 / 4];

    vu_reg_t vu_regs[32];

    struct {
        vu_reg_t l;
        vu_reg_t h;
    } vcc;

    struct {
        vu_reg_t l;
        vu_reg_t h;
    } vco;

    vu_reg_t vce;

    struct {
        vu_reg_t h;
        vu_reg_t m;
        vu_reg_t l;
    } acc;

    int sync; // For syncing RSP with CPU

    shalf divin;
    bool divin_loaded;
    shalf divout;

    bool semaphore_held;

    byte (*read_byte)(word);
    void (*write_byte)(word, byte);

    byte (*read_physical_byte)(word);
    void (*write_physical_byte)(word, byte);

    half (*read_half)(word);
    void (*write_half)(word, half);

    word (*read_word)(word);
    void (*write_word)(word, word);

    word (*read_physical_word)(word);
    void (*write_physical_word)(word, word);

    //dword (*read_dword)(word);
    //void (*write_dword)(word, dword);
} rsp_t;

INLINE void set_rsp_pc(rsp_t* rsp, half pc) {
    rsp->pc = pc >> 2;
    rsp->next_pc = rsp->pc + 1;
}

#endif //N64_RSP_TYPES_H
