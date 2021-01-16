#ifndef N64_DYNAREC_H
#define N64_DYNAREC_H

#include <system/n64system.h>
#include <dynasm/dasm_proto.h>

// 4KiB aligned pages
#define BLOCKCACHE_OUTER_SHIFT 12
#define BLOCKCACHE_PAGE_SIZE (1 << BLOCKCACHE_OUTER_SHIFT)
#define BLOCKCACHE_OUTER_SIZE (0x80000000 >> BLOCKCACHE_OUTER_SHIFT)
// word aligned instructions
#define BLOCKCACHE_INNER_SIZE (BLOCKCACHE_PAGE_SIZE >> 2)

typedef enum dynarec_instruction_category {
    NORMAL,
    STORE,
    BRANCH,
    BRANCH_LIKELY,
    TLB_WRITE,
    ERET
} dynarec_instruction_category_t;

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

typedef void(*mipsinstr_compiler_t)(dasm_State**, mips_instruction_t, word, int*, int, word*);

typedef struct dynarec_ir {
    dynarec_instruction_category_t category;
    instruction_format_t format;
    bool exception_possible;
    mipsinstr_compiler_t compiler;
} dynarec_ir_t;

typedef struct n64_dynarec_block {
    int (*run)(r4300i_t* cpu);
} n64_dynarec_block_t;

typedef struct n64_dynarec {
    byte* codecache;
    dword codecache_size;
    dword codecache_used;

    n64_dynarec_block_t* blockcache[BLOCKCACHE_OUTER_SIZE];
} n64_dynarec_t;

int n64_dynarec_step(n64_system_t* system, n64_dynarec_t* dynarec);
n64_dynarec_t* n64_dynarec_init(n64_system_t* system, byte* codecache, size_t codecache_size);
void invalidate_dynarec_page(n64_dynarec_t* dynarec, word physical_address);

#endif //N64_DYNAREC_H
