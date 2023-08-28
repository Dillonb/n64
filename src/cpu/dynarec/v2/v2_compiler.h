#ifndef N64_V2_COMPILER_H
#define N64_V2_COMPILER_H

#include <dynarec/dynarec.h>

INLINE bool is_memory(u64 address) {
    return false; // TODO
}

typedef struct source_instruction {
    mips_instruction_t instr;
    dynarec_instruction_category_t category;
} source_instruction_t;

// Extra slot for the edge case where the branch delay slot is in the next page
#define TEMP_CODE_SIZE (BLOCKCACHE_INNER_SIZE + 1)
#define MAX_BLOCK_LENGTH BLOCKCACHE_INNER_SIZE

extern int temp_code_len;
extern u64 temp_code_vaddr;
extern source_instruction_t temp_code[TEMP_CODE_SIZE];

bool should_break(u32 address);
u64 resolve_virtual_address_for_jit(u64 virtual, u64 except_pc, bus_access_t bus_access);
u64 v2_get_last_compiled_block();
void v2_compile_new_block(n64_dynarec_block_t *block, bool *code_mask, u64 virtual_address, u32 physical_address);
void v2_compiler_init();
void v2_set_idle_loop_detection_enabled(bool enabled);

#endif // N64_V2_COMPILER_H