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
#define IS_PAGE_BOUNDARY(address) (((address) & (BLOCKCACHE_PAGE_SIZE - 1)) == 0)


typedef enum dynarec_instruction_category {
    NORMAL,
    CACHE,
    STORE,
    BRANCH,
    BRANCH_LIKELY,
    TLB_WRITE,
    BLOCK_ENDER
} dynarec_instruction_category_t;

INLINE bool is_branch(dynarec_instruction_category_t category) {
    return category == BRANCH || category == BRANCH_LIKELY;
}

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
    N64DYNAREC->blockcache[outer_index] = NULL;
}

INLINE bool is_code(u32 physical_address) {
    bool* code_mask = N64DYNAREC->code_mask[physical_address >> BLOCKCACHE_OUTER_SHIFT];
    return code_mask != NULL && code_mask[BLOCKCACHE_INNER_INDEX(physical_address)];
}

INLINE void invalidate_dynarec_page(u32 physical_address) {
    if (unlikely(is_code(physical_address))) {
        invalidate_dynarec_page_by_index(dynarec_outer_index(physical_address));
    }
}

int n64_dynarec_step();
n64_dynarec_t* n64_dynarec_init(u8* codecache, size_t codecache_size);
void invalidate_dynarec_page(u32 physical_address);
void invalidate_dynarec_all_pages();

#endif //N64_DYNAREC_H
