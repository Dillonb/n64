#ifndef N64_TLB_INSTRUCTIONS_H
#define N64_TLB_INSTRUCTIONS_H

#include "r4300i.h"
#include "mips_instruction_decode.h"
#include <mem/n64bus.h>

#ifndef MIPS_INSTR
#define MIPS_INSTR(NAME) void NAME(mips_instruction_t instruction)
#endif

INLINE void clear_tlb_cache(tlb_entry_t entry) {
    if (entry.initialized) {
        u64 page_mask = entry.page_mask.raw | 0x1FFF;
        u64 start = entry.entry_hi.raw & ~page_mask;
        u64 end = start | page_mask;

        u32 start_index = GET_TLB_CACHE_INDEX(start);
        u32 end_index = GET_TLB_CACHE_INDEX(end);

        for (u32 index = start_index; index <= end_index; index++) {
            memset(N64CP0.tlb_cache[index], 0, TLB_CACHE_ASSOCIATIVITY * sizeof(tlb_cache_entry_t));
        }
    }
}

void do_tlbwi(int index);
MIPS_INSTR(mips_tlbwi);
void do_tlbp();
MIPS_INSTR(mips_tlbp);
void do_tlbr();
MIPS_INSTR(mips_tlbr);
MIPS_INSTR(mips_tlbwr);

#endif //N64_TLB_INSTRUCTIONS_H
