#include "tlb_instructions.h"
#include "r4300i_register_access.h"
#include <mem/n64bus.h>

// Loads the contents of the pfn Hi, pfn Lo0, pfn Lo1, and page mask
// registers to the TLB pfn indicated by the index register.
MIPS_INSTR(mips_tlbwi) {
    int index = N64CP0.index & 0x3F;

    do_tlbwi(index);
}

// Loads the address of the TLB pfn coinciding with the contents of the pfn
// Hi register to the index register.
void do_tlbp() {
    int match = -1;
    tlb_entry_t* entry = find_tlb_entry(N64CP0.entry_hi.raw, &match);
    if (entry && match >= 0) {
        N64CP0.index = match;
    } else {
        N64CP0.index = 0x80000000;
    }
}
MIPS_INSTR(mips_tlbp) {
    do_tlbp();
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
    do_tlbwi(get_cp0_random());
}
