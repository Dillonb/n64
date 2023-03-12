#ifndef N64_TLB_INSTRUCTIONS_H
#define N64_TLB_INSTRUCTIONS_H

#include "r4300i.h"
#include "mips_instruction_decode.h"

#ifndef MIPS_INSTR
#define MIPS_INSTR(NAME) void NAME(mips_instruction_t instruction)
#endif

INLINE void do_tlbwi(int index) {
    cp0_page_mask_t page_mask;
    page_mask = N64CP0.page_mask;

    // For each pair of bits:
    // 00 -> 00
    // 01 -> 00
    // 10 -> 11
    // 11 -> 11
    // The top bit sets the value of both bits.
    u32 top = page_mask.mask & 0b101010101010;
    page_mask.mask = top | (top >> 1);

    if (index >= 32) {
        logfatal("TLBWI to TLB index %d", index);
    }
    N64CP0.tlb[index].entry_hi.raw  = N64CP0.entry_hi.raw;
    N64CP0.tlb[index].entry_hi.vpn2 &= ~page_mask.mask;
    // Note: different masks than the Cop0 registers for entry_lo0 and 1, so another mask is needed here
    N64CP0.tlb[index].entry_lo0.raw = N64CP0.entry_lo0.raw & 0x03FFFFFE;
    N64CP0.tlb[index].entry_lo1.raw = N64CP0.entry_lo1.raw & 0x03FFFFFE;
    N64CP0.tlb[index].page_mask.raw = page_mask.raw;

    N64CP0.tlb[index].global = N64CP0.entry_lo0.g && N64CP0.entry_lo1.g;

    N64CP0.tlb[index].initialized = true;

}

void do_tlbp();
MIPS_INSTR(mips_tlbwi);
MIPS_INSTR(mips_tlbp);
MIPS_INSTR(mips_tlbr);
MIPS_INSTR(mips_tlbwr);

#endif //N64_TLB_INSTRUCTIONS_H
