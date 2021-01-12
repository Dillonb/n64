#include "dynarec_memory_management.h"
#include "dynarec.h"

void flush_code_cache(n64_dynarec_t* dynarec) {
    // Just set the pointer back to the beginning, no need to clear the actual data.
    dynarec->codecache_used = 0;

    // However, the block cache needs to be fully invalidated.
    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        dynarec->blockcache[i] = NULL;
    }
}

void* dynarec_bumpalloc(n64_dynarec_t* dynarec, size_t size) {
    if (dynarec->codecache_used + size >= dynarec->codecache_size) {
        flush_code_cache(dynarec);
    }

    void* ptr = &dynarec->codecache[dynarec->codecache_used];

    dynarec->codecache_used += size;

#ifdef N64_LOG_COMPILATIONS
    printf("bumpalloc: %ld used of %ld\n", dynarec->codecache_used, dynarec->codecache_size);
#endif

    return ptr;
}

void* dynarec_bumpalloc_zero(n64_dynarec_t* dynarec, size_t size) {
    byte* ptr = dynarec_bumpalloc(dynarec, size);

    for (int i = 0; i < size; i++) {
        ptr[i] = 0;
    }

    return ptr;
}
