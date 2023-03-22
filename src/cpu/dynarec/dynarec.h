#ifndef N64_DYNAREC_H
#define N64_DYNAREC_H

#include <system/n64system.h>
#include <dynasm/dasm_proto.h>
#include <common/util.h>

// 4KiB aligned pages
#define BLOCKCACHE_OUTER_SHIFT 12
#define BLOCKCACHE_PAGE_SIZE (1 << BLOCKCACHE_OUTER_SHIFT)
#define BLOCKCACHE_OUTER_SIZE (0x80000000 >> BLOCKCACHE_OUTER_SHIFT)
// word aligned instructions
#define BLOCKCACHE_INNER_SIZE (BLOCKCACHE_PAGE_SIZE >> 2)
#define BLOCKCACHE_INNER_INDEX(physical) (((physical) & (BLOCKCACHE_PAGE_SIZE - 1)) >> 2)

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

typedef struct n64_dynarec_block {
    int (*run)(r4300i_t* cpu);
} n64_dynarec_block_t;

typedef struct n64_dynarec {
    u8* codecache;
    u64 codecache_size;
    u64 codecache_used;

    n64_dynarec_block_t* blockcache[BLOCKCACHE_OUTER_SIZE];
    bool* code_mask[BLOCKCACHE_OUTER_SIZE];
} n64_dynarec_t;

INLINE u32 dynarec_outer_index(u32 physical_address) {
    return physical_address >> BLOCKCACHE_OUTER_SHIFT;
}

INLINE void invalidate_dynarec_page_by_index(u32 outer_index) {
#ifdef ENABLE_DYNAREC
    N64DYNAREC->blockcache[outer_index] = NULL;
#endif
}

INLINE bool is_code(u32 physical_address) {
    bool* code_mask = N64DYNAREC->code_mask[physical_address >> BLOCKCACHE_OUTER_SHIFT];
    return code_mask != NULL && code_mask[BLOCKCACHE_INNER_INDEX(physical_address)];
}

INLINE void invalidate_dynarec_page(u32 physical_address) {
    #ifdef ENABLE_DYNAREC
    if (unlikely(is_code(physical_address))) {
        invalidate_dynarec_page_by_index(dynarec_outer_index(physical_address));
    }
    #endif
}

#ifdef ENABLE_DYNAREC
int n64_dynarec_step();
n64_dynarec_t* n64_dynarec_init(u8* codecache, size_t codecache_size);
void invalidate_dynarec_page(u32 physical_address);
void invalidate_dynarec_all_pages();
#else
INLINE void no_dynarec_err() { 
    logfatal("Dynarec is disabled at compile time");
}
INLINE int n64_dynarec_step() { no_dynarec_err(); }
INLINE n64_dynarec_t* n64_dynarec_init(u8* codecache, size_t codecache_size) { no_dynarec_err(); }
INLINE void invalidate_dynarec_all_pages() { }
#endif

#endif //N64_DYNAREC_H
