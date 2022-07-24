#ifndef N64_RSP_DYNAREC_H
#define N64_RSP_DYNAREC_H

#include <util.h>
#include <stdlib.h>
#include <cpu/rsp_types.h>

// Temporarily just the same size as IMEM
#define RSP_BLOCKCACHE_SIZE (0x1000 / 4)

typedef struct rsp rsp_t;

typedef struct rsp_dynarec_block {
    int (*run)(rsp_t* cpu);
} rsp_dynarec_block_t;

typedef struct rsp_dynarec {
    u8* codecache;
    u64 codecache_size;
    u64 codecache_used;

    rsp_dynarec_block_t blockcache[RSP_BLOCKCACHE_SIZE];
} rsp_dynarec_t;

rsp_dynarec_t* rsp_dynarec_init(u8* codecache, size_t codecache_size);
int rsp_dynarec_step();

#endif //N64_RSP_DYNAREC_H
