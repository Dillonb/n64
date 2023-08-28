#ifndef N64_V2_COMPILER_X64_H
#define N64_V2_COMPILER_X64_H
#include <dynarec/dynarec.h>

void v2_emit_block(n64_dynarec_block_t* block, u32 physical_address);
void v2_compiler_init_platformspecific();

#endif //N64_V2_COMPILER_X64_H