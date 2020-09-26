#include "dynarec.h"

#include <sys/mman.h>
#include <mem/n64bus.h>

void compile_new_block(n64_dynarec_t* dynarec, word pc) {
    logfatal("Compiling new block at 0x%08X", pc);
}

void* bumpalloc(n64_dynarec_t* dynarec, size_t size) {
    if (dynarec->codecache_used + size >= dynarec->codecache_size) {
        logfatal("Exhausted code cache!");
    }

    void* ptr = &dynarec->codecache[dynarec->codecache_used];

    dynarec->codecache_used += size;

    return ptr;
}

void* bumpalloc_zero(n64_dynarec_t* dynarec, size_t size) {
    printf("bumpalloc\n");
    byte* ptr = bumpalloc(dynarec, size);

    for (int i = 0; i < size; i++) {
        ptr[i] = 0;
    }

    return ptr;
}

int n64_dynarec_step(n64_system_t* system, n64_dynarec_t* dynarec) {
    word physical = resolve_virtual_address(system->cpu.pc, &system->cpu.cp0);
    word outer_index = physical >> BLOCKCACHE_OUTER_SHIFT;
    n64_dynarec_block_t* block_list = dynarec->blockcache[outer_index];

    if (block_list == NULL) {
        block_list = bumpalloc_zero(dynarec, BLOCKCACHE_INNER_SIZE);
        dynarec->blockcache[outer_index] = block_list;
    }

    word inner_index = (physical & (BLOCKCACHE_INNER_SIZE - 1)) >> 2;

    n64_dynarec_block_t block = block_list[inner_index];

    if (block.run != NULL) {
        block.run(&system->cpu);
    } else {
        compile_new_block(dynarec, physical);
    }

    return 0;
}

n64_dynarec_t* n64_dynarec_init(n64_system_t* system, byte* codecache, size_t codecache_size) {
    printf("Trying to malloc %ld bytes\n", sizeof(n64_dynarec_t));
    n64_dynarec_t* dynarec = calloc(1, sizeof(n64_dynarec_t));

    dynarec->codecache_size = codecache_size;
    dynarec->codecache_used = 0;

    for (int i = 0; i < codecache_size; i++) {
        codecache[i] = 0;
    }

    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        dynarec->blockcache[i] = 0;
    }

    dynarec->codecache = codecache;
    return dynarec;
}
