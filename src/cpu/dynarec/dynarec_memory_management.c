#include <rsp.h>
#include "dynarec_memory_management.h"
#include "dynarec.h"

void flush_code_cache() {
    // Just set the pointer back to the beginning, no need to clear the actual data.
    n64dynarec.codecache_used = 0;

    // However, the block cache needs to be fully invalidated.
    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        n64dynarec.blockcache[i] = NULL;
    }
}

void flush_rsp_code_cache() {
    logalways("Flushing RSP code cache!");
    // Just set the pointer back to the beginning, no need to clear the actual data.
    N64RSPDYNAREC->codecache_used = 0;

    // However, the block cache needs to be fully invalidated.
    reset_rsp_dynarec_code_overlays(N64RSPDYNAREC);
}

void* dynarec_bumpalloc(size_t size) {
    if (n64dynarec.codecache_used + size >= n64dynarec.codecache_size) {
        flush_code_cache();
    }

    void* ptr = &n64dynarec.codecache[n64dynarec.codecache_used];

    n64dynarec.codecache_used += size;

#ifdef N64_LOG_COMPILATIONS
    printf("bumpalloc: %ld used of %ld\n", n64dynarec.codecache_used, n64dynarec.codecache_size);
#endif

    return ptr;
}

// TODO: this should take a size so that we can flush here if necessary, to guarantee the pointer is correct.
void* dynarec_bumpalloc_get_next_allocation_ptr() {
    return &n64dynarec.codecache[n64dynarec.codecache_used];
}

void* dynarec_bumpalloc_zero(size_t size) {
    u8* ptr = dynarec_bumpalloc(size);

    memset(ptr, 0, size);

    return ptr;
}

void* rsp_dynarec_bumpalloc(size_t size) {
    if (N64RSPDYNAREC->codecache_used + size >= N64RSPDYNAREC->codecache_size) {
        flush_rsp_code_cache();
    }

    void* ptr = &N64RSPDYNAREC->codecache[N64RSPDYNAREC->codecache_used];

    N64RSPDYNAREC->codecache_used += size;

#ifdef N64_LOG_COMPILATIONS
    printf("bumpalloc: %ld used of %ld\n", n64dynarec.codecache_used, n64dynarec.codecache_size);
#endif

    return ptr;
}

// TODO: this should take a size so that we can flush here if necessary, to guarantee the pointer is correct.
void* rsp_dynarec_bumpalloc_get_next_allocation_ptr() {
    return &N64RSPDYNAREC->codecache[N64RSPDYNAREC->codecache_used];
}
