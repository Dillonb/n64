#ifndef N64_DYNAREC_MEMORY_MANAGEMENT_H
#define N64_DYNAREC_MEMORY_MANAGEMENT_H

#ifdef __APPLE__
#include <pthread.h>
#endif

#include "dynarec.h"

void* dynarec_bumpalloc(size_t size);
void* dynarec_bumpalloc_get_next_allocation_ptr();
void* dynarec_bumpalloc_zero(size_t size);
void* rsp_dynarec_bumpalloc(size_t size);

#ifdef __APPLE__
#define CODECACHE_ALLOW_WRITES() do { pthread_jit_write_protect_np(false); } while (0)
#define CODECACHE_ALLOW_EXEC() do { pthread_jit_write_protect_np(true); } while (0)
#else
#define CODECACHE_ALLOW_WRITES() do { } while (0)
#define CODECACHE_ALLOW_EXEC() do { } while (0)
#endif

#endif //N64_DYNAREC_MEMORY_MANAGEMENT_H
