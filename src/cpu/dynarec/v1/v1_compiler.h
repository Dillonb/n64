#ifndef N64_V1_COMPILER_H
#define N64_V1_COMPILER_H

#include <dynarec/dynarec.h>

typedef enum dynarec_instruction_category {
    NORMAL,
    STORE,
    BRANCH,
    BRANCH_LIKELY,
    TLB_WRITE,
    BLOCK_ENDER
} dynarec_instruction_category_t;

INLINE bool is_branch(dynarec_instruction_category_t category) {
    return category == BRANCH || category == BRANCH_LIKELY;
}

typedef enum instruction_format {
    CALL_INTERPRETER,
    FORMAT_NOP,
    SHIFT_CONST,
    I_TYPE,
    R_TYPE,
    J_TYPE,
    MF_MULTREG,
    MT_MULTREG
} instruction_format_t;

typedef void(*mipsinstr_compiler_t)(dasm_State**, mips_instruction_t, u32, int*, int, u32*);

typedef struct dynarec_ir {
    dynarec_instruction_category_t category;
    instruction_format_t format;
    bool exception_possible;
    mipsinstr_compiler_t compiler;
} dynarec_ir_t;


void v1_compile_new_block(n64_dynarec_block_t* block, bool* code_mask, u64 virtual_address, u32 physical_address);
void v1_compiler_init();

#endif // N64_V1_COMPILER_H