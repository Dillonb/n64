#ifndef N64_DYNAREC_MEMORY_MANAGEMENT_H
#define N64_DYNAREC_MEMORY_MANAGEMENT_H

#include "dynarec.h"

void* dynarec_bumpalloc(n64_dynarec_t* dynarec, size_t size);
void* dynarec_bumpalloc_zero(n64_dynarec_t* dynarec, size_t size);
#endif //N64_DYNAREC_MEMORY_MANAGEMENT_H
