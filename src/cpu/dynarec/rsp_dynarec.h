#ifndef N64_RSP_DYNAREC_H
#define N64_RSP_DYNAREC_H

#include <util.h>
#include <stdlib.h>
#include <cpu/rsp_types.h>

// the same size as IMEM
#define RSP_BLOCKCACHE_SIZE (0x1000 / 4)
// TODO tune - games i've tested seem to use 8-10
#define RSP_NUM_CODE_OVERLAYS 20

typedef struct rsp rsp_t;

typedef struct rsp_dynarec_block {
    int (*run)(rsp_t* cpu);
} rsp_dynarec_block_t;

typedef struct rsp_code_overlay {
    // Is the value at this index code? (Has it been compiled into a block indexed by this blockcache?)
    // value is either 0 or 0xFFFFFFFF - to make checking this with SIMD instructions easier
    u32 code_mask[RSP_BLOCKCACHE_SIZE];
    // A copy of only the parts of IMEM that have been compiled into code.
    u32 code[RSP_BLOCKCACHE_SIZE];
    // Mapping of index -> executable block
    rsp_dynarec_block_t blockcache[RSP_BLOCKCACHE_SIZE];
} rsp_code_overlay_t;

typedef struct rsp_dynarec {
    u8* codecache;
    u64 codecache_size;
    u64 codecache_used;

    rsp_code_overlay_t code_overlays[RSP_NUM_CODE_OVERLAYS];
    int selected_code_overlay;
    int code_overlays_allocated;
    bool dirty;
} rsp_dynarec_t;

void reset_rsp_dynarec_code_overlays(rsp_dynarec_t* dynarec);
rsp_dynarec_t* rsp_dynarec_init(u8* codecache, size_t codecache_size);
int rsp_dynarec_step();
int rsp_missing_block_handler();

#endif //N64_RSP_DYNAREC_H
