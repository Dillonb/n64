#ifndef N64_V2_COMPILER_H
#define N64_V2_COMPILER_H

#include <dynarec/dynarec.h>

void print_ir_block();
u64 v2_get_last_compiled_block();
void v2_compile_new_block(n64_dynarec_block_t *block, bool *code_mask, u64 virtual_address, u32 physical_address);
void v2_compiler_init();
void v2_set_idle_loop_detection_enabled(bool enabled);

#endif // N64_V2_COMPILER_H