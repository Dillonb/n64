#include "tlb_instructions.h"
#include "r4300i_register_access.h"
#include <mem/n64bus.h>

INLINE void tlbwi(int index) {
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

// Loads the contents of the pfn Hi, pfn Lo0, pfn Lo1, and page mask
// registers to the TLB pfn indicated by the index register.
MIPS_INSTR(mips_tlbwi) {
    int index = N64CP0.index & 0x3F;

    tlbwi(index);
}

// Loads the address of the TLB pfn coinciding with the contents of the pfn
// Hi register to the index register.
MIPS_INSTR(mips_tlbp) {
    int match = -1;
    tlb_entry_t* entry = find_tlb_entry(N64CP0.entry_hi.raw, &match);
    if (entry && match >= 0) {
        N64CP0.index = match;
    } else {
        N64CP0.index = 0x80000000;
    }
}

MIPS_INSTR(mips_tlbr) {
    unimplemented(N64CP0.is_64bit_addressing, "TLBR in 64 bit mode!");
    int index = N64CP0.index & 0b111111;
    if (index >= 32) {
        logfatal("TLBR from TLB index %d", index);
    }

    tlb_entry_t entry = N64CP0.tlb[index];

    N64CP0.entry_hi.raw  = entry.entry_hi.raw;
    N64CP0.entry_lo0.raw = entry.entry_lo0.raw & CP0_ENTRY_LO_WRITE_MASK;
    N64CP0.entry_lo1.raw = entry.entry_lo1.raw & CP0_ENTRY_LO_WRITE_MASK;

    N64CP0.entry_lo0.g = entry.global;
    N64CP0.entry_lo1.g = entry.global;
    N64CP0.page_mask.raw = entry.page_mask.raw;
}

MIPS_INSTR(mips_tlbwr) {
    tlbwi(get_cp0_random());
}
