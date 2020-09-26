#ifndef N64_DYNAREC_H
#define N64_DYNAREC_H

#include <system/n64system.h>

// 4KiB aligned pages
#define BLOCKCACHE_OUTER_SHIFT 12
#define BLOCKCACHE_PAGE_SIZE (1 << BLOCKCACHE_OUTER_SHIFT)
#define BLOCKCACHE_OUTER_SIZE (0x80000000 >> BLOCKCACHE_OUTER_SHIFT)
// word aligned instructions
#define BLOCKCACHE_INNER_SIZE (BLOCKCACHE_PAGE_SIZE >> 2)

typedef struct n64_dynarec_block {
    word start_address;
    word length;
    void (*run)(r4300i_t* cpu);
} n64_dynarec_block_t;

typedef struct n64_dynarec {
    byte* codecache;
    dword codecache_size;
    dword codecache_used;

    n64_dynarec_block_t* blockcache[BLOCKCACHE_OUTER_SIZE];
} n64_dynarec_t;

int n64_dynarec_step(n64_system_t* system, n64_dynarec_t* dynarec);
n64_dynarec_t* n64_dynarec_init(n64_system_t* system, byte* codecache, size_t codecache_size);
void invalidate_dynarec_page(n64_dynarec_t* dynarec, word physical_address);

#endif //N64_DYNAREC_H
