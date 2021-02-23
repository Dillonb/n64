#include "dynarec_memory_management.h"
#include "dynarec.h"

void flush_code_cache() {
    // Just set the pointer back to the beginning, no need to clear the actual data.
    N64DYNAREC->codecache_used = 0;

    // However, the block cache needs to be fully invalidated.
    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        N64DYNAREC->blockcache[i] = NULL;
    }
}

void* dynarec_bumpalloc(size_t size) {
    if (N64DYNAREC->codecache_used + size >= N64DYNAREC->codecache_size) {
        flush_code_cache();
    }

    void* ptr = &N64DYNAREC->codecache[N64DYNAREC->codecache_used];

    N64DYNAREC->codecache_used += size;

#ifdef N64_LOG_COMPILATIONS
    printf("bumpalloc: %ld used of %ld\n", N64DYNAREC->codecache_used, N64DYNAREC->codecache_size);
#endif

    return ptr;
}

void* dynarec_bumpalloc_zero(size_t size) {
    byte* ptr = dynarec_bumpalloc(size);

    for (int i = 0; i < size; i++) {
        ptr[i] = 0;
    }

    return ptr;
}
