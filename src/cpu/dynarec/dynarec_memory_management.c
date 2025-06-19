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

#ifdef N64_DYNAREC_V1_ENABLED
void flush_rsp_code_cache() {
    logalways("Flushing RSP code cache!");
    // Just set the pointer back to the beginning, no need to clear the actual data.
    N64RSPDYNAREC->codecache_used = 0;

    // However, the block cache needs to be fully invalidated.
    reset_rsp_dynarec_code_overlays(N64RSPDYNAREC);
}
#endif

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

void* dynarec_bumpalloc_get_next_allocation_ptr() {
    return &n64dynarec.codecache[n64dynarec.codecache_used];
}

void* dynarec_bumpalloc_zero(size_t size) {
    u8* ptr = dynarec_bumpalloc(size);

    for (int i = 0; i < size; i++) {
        ptr[i] = 0;
    }

    return ptr;
}

#ifdef N64_DYNAREC_V1_ENABLED
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
#endif
