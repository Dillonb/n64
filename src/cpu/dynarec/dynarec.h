#ifndef N64_DYNAREC_H
#define N64_DYNAREC_H

#ifdef __cplusplus
extern "C" {
#endif

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
#define BLOCKCACHE_OUTER_INDEX(physical) ((physical) >> BLOCKCACHE_OUTER_SHIFT)
#define IS_PAGE_BOUNDARY(address) (((address) & (BLOCKCACHE_PAGE_SIZE - 1)) == 0)
#define INDICES_TO_ADDRESS(outer, inner) (((outer) << BLOCKCACHE_OUTER_SHIFT) | ((inner) << 2))


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
    size_t guest_size;
    size_t host_size;
} n64_dynarec_block_t;

typedef struct n64_dynarec {
    int (*run_block)(u64 block_addr);
    u8* codecache;
    u64 codecache_size;
    u64 codecache_used;

    n64_dynarec_block_t* blockcache[BLOCKCACHE_OUTER_SIZE];
    bool* code_mask[BLOCKCACHE_OUTER_SIZE];
} n64_dynarec_t;

extern n64_dynarec_t n64dynarec;

INLINE void invalidate_dynarec_page_by_index(u32 outer_index) {
    n64dynarec.blockcache[outer_index] = NULL;
}

INLINE bool is_code(u32 physical_address) {
    bool* code_mask = n64dynarec.code_mask[physical_address >> BLOCKCACHE_OUTER_SHIFT];
    return code_mask != NULL && code_mask[BLOCKCACHE_INNER_INDEX(physical_address)];
}

INLINE void invalidate_dynarec_page(u32 physical_address) {
    if (unlikely(is_code(physical_address))) {
        invalidate_dynarec_page_by_index(BLOCKCACHE_OUTER_INDEX(physical_address));
    }
}

int n64_dynarec_step();
void n64_dynarec_init(u8* codecache, size_t codecache_size);
void invalidate_dynarec_page(u32 physical_address);
void invalidate_dynarec_all_pages();
int missing_block_handler();

#ifdef __cplusplus
}
#endif

#endif //N64_DYNAREC_H
