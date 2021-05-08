#include "tlb_instructions.h"
#include "r4300i_register_access.h"
#include <mem/n64bus.h>

void tlbwi_32b(int index) {
    cp0_entry_hi_t entry_hi;
    entry_hi.raw = N64CP0.entry_hi.raw   & 0xFFFFE0FF;

    cp0_entry_lo_t entry_lo0;
    entry_lo0.raw = N64CP0.entry_lo0.raw & 0x3FFFFFFF;

    cp0_entry_lo_t entry_lo1;
    entry_lo1.raw = N64CP0.entry_lo1.raw & 0x3FFFFFFF;

    cp0_page_mask_t page_mask;
    page_mask.raw = N64CP0.page_mask.raw & 0x01FFE000;

    if (index >= 32) {
        logfatal("TLBWI to TLB index %d", index);
    }
    N64CP0.tlb[index].entry_hi.raw  = entry_hi.raw;
    N64CP0.tlb[index].entry_lo0.raw = entry_lo0.raw;
    N64CP0.tlb[index].entry_lo1.raw = entry_lo1.raw;
    N64CP0.tlb[index].page_mask.raw = page_mask.raw;

    N64CP0.tlb[index].global = entry_lo0.g && entry_lo1.g;
    N64CP0.tlb[index].valid  = entry_lo0.v || entry_lo1.v;
    N64CP0.tlb[index].asid   = entry_hi.asid;

}

void tlbwi_64b(int index) {
    cp0_entry_hi_64_t entry_hi;
    entry_hi.raw = N64CP0.entry_hi_64.raw & 0xC00000FFFFFFE0FF;

    cp0_entry_lo_t entry_lo0;
    entry_lo0.raw = N64CP0.entry_lo0.raw & 0x3FFFFFFF;

    cp0_entry_lo_t entry_lo1;
    entry_lo1.raw = N64CP0.entry_lo1.raw & 0x3FFFFFFF;

    cp0_page_mask_t page_mask;
    page_mask.raw = N64CP0.page_mask.raw & 0x01FFE000;

    if (index >= 32) {
        logfatal("TLBWI to TLB index %d", index);
    }

    N64CP0.tlb_64[index].entry_hi.raw  = entry_hi.raw;
    N64CP0.tlb_64[index].entry_lo0.raw = entry_lo0.raw;
    N64CP0.tlb_64[index].entry_lo1.raw = entry_lo1.raw;
    N64CP0.tlb_64[index].page_mask.raw = page_mask.raw;

    N64CP0.tlb_64[index].global = entry_lo0.g && entry_lo1.g;
    N64CP0.tlb_64[index].valid  = entry_lo0.v || entry_lo1.v;
    N64CP0.tlb_64[index].asid   = entry_hi.asid;
    N64CP0.tlb_64[index].region = entry_hi.r;
}

// Loads the contents of the pfn Hi, pfn Lo0, pfn Lo1, and page mask
// registers to the TLB pfn indicated by the index register.
MIPS_INSTR(mips_tlbwi) {
    // TODO - these are mostly identical, can collapse

    int index = N64CP0.index & 0x3F;

    if (N64CP0.is_64bit_addressing) {
        tlbwi_64b(index);
    } else {
        tlbwi_32b(index);
    }
}

// Loads the address of the TLB pfn coinciding with the contents of the pfn
// Hi register to the index register.
MIPS_INSTR(mips_tlbp) {
    unimplemented(N64CP0.is_64bit_addressing, "TLBP in 64 bit mode!");
    word entry_hi = N64CP0.entry_hi.raw   & 0xFFFFE0FF;
    int match;
    if (tlb_probe(entry_hi, NULL, &match)) {
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

    N64CP0.entry_hi.raw  = entry.entry_hi.raw & ~(entry.page_mask.raw);
    N64CP0.entry_lo0.raw = entry.entry_lo0.raw;
    N64CP0.entry_lo1.raw = entry.entry_lo1.raw;

    N64CP0.entry_lo0.g = entry.entry_hi.g;
    N64CP0.entry_lo1.g = entry.entry_hi.g;
}

MIPS_INSTR(mips_tlbwr) {
    unimplemented(N64CP0.is_64bit_addressing, "tlbwr in 64 bit mode");
    tlbwi_32b(get_cp0_random());
}
