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

#define SP_DMEM_SIZE 0x1000
#define SP_IMEM_SIZE 0x1000

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

ASSERTWORD(rsp_status_t);

typedef struct rsp rsp_t;

void cache_rsp_instruction(rsp_t* rsp, mips_instruction_t instr);

typedef void(*rspinstr_handler_t)(rsp_t*, mips_instruction_t);

typedef struct rsp_icache_entry {
    mips_instruction_t instruction;
    rspinstr_handler_t handler;
} rsp_icache_entry_t;

typedef union mem_addr {
    word raw;
    struct {
        unsigned address:12;
        bool imem:1;
        unsigned:19;
    };
} mem_addr_t;

ASSERTWORD(mem_addr_t);

typedef union dram_addr {
    word raw;
    struct {
        unsigned address:24;
        unsigned:8;
    };
} dram_addr_t;

ASSERTWORD(dram_addr_t);

typedef struct rsp {
    word gpr[32];
    half prev_pc;
    half pc;
    half next_pc;
    //dword mult_hi;
    //dword mult_lo;

    int steps;

    vecr zero;

    rsp_status_t status;

    struct {
        mem_addr_t mem_addr;
        dram_addr_t dram_addr;

        // values are stored in shadow registers until the DMA actually runs
        mem_addr_t shadow_mem_addr;
        dram_addr_t shadow_dmem_addr;
        union {
            struct {
                unsigned length: 12;
                unsigned count: 8;
                unsigned skip: 12;
            };
            word raw;
        } dma;
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

    byte sp_dmem[SP_DMEM_SIZE];
    byte sp_imem[SP_IMEM_SIZE];

    word (*read_physical_word)(word);
    void (*write_physical_word)(word, word);
} rsp_t;

INLINE void set_rsp_pc(rsp_t* rsp, half pc) {
    rsp->pc = pc >> 2;
    rsp->next_pc = rsp->pc + 1;
}

#endif //N64_RSP_TYPES_H
