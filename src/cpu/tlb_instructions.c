#include "tlb_instructions.h"
#include <mem/n64bus.h>

// Loads the contents of the entry Hi, entry Lo0, entry Lo1, and page mask
// registers to the TLB entry indicated by the index register.
MIPS_INSTR(mips_tlbwi) {
    cp0_entry_hi_t entry_hi;
    entry_hi.raw = cpu->cp0.entry_hi.raw   & 0xFFFFE0FF;

    cp0_entry_lo_t entry_lo0;
    entry_lo0.raw = cpu->cp0.entry_lo0.raw & 0x7FFFFFFF;

    cp0_entry_lo_t entry_lo1;
    entry_lo1.raw = cpu->cp0.entry_lo1.raw & 0x7FFFFFFF;

    cp0_page_mask_t page_mask;
    page_mask.raw = cpu->cp0.page_mask.raw & 0x01FFE000;

    int index = cpu->cp0.index & 0x3F;

    if (index >= 32) {
        logfatal("TLBWI to TLB index %d", index);
    }
    cpu->cp0.tlb[index].entry_hi.raw  = entry_hi.raw;
    cpu->cp0.tlb[index].entry_lo0.raw = entry_lo0.raw;
    cpu->cp0.tlb[index].entry_lo1.raw = entry_lo1.raw;
    cpu->cp0.tlb[index].page_mask.raw = page_mask.raw;

    cpu->cp0.tlb[index].global = entry_lo0.g && entry_lo1.g;
    cpu->cp0.tlb[index].valid  = entry_lo0.v || entry_lo1.v;
    cpu->cp0.tlb[index].asid   = entry_hi.asid;

}

// Loads the address of the TLB entry coinciding with the contents of the entry
// Hi register to the index register.
MIPS_INSTR(mips_tlbp) {
    word entry_hi = cpu->cp0.entry_hi.raw   & 0xFFFFE0FF;
    int match;
    if (tlb_probe(entry_hi, NULL, &match, &cpu->cp0)) {
        cpu->cp0.index = match;
    } else {
        cpu->cp0.index = 0x80000000;
    }
}

MIPS_INSTR(mips_tlbr) {
        int index = cpu->cp0.index & 0b111111;
        if (index >= 32) {
            logfatal("TLBR from TLB index %d", index);
        }

        tlb_entry_t entry = cpu->cp0.tlb[index];

        cpu->cp0.entry_hi.raw  = entry.entry_hi.raw & ~(entry.page_mask.raw);
        cpu->cp0.entry_lo0.raw = entry.entry_lo0.raw;
        cpu->cp0.entry_lo1.raw = entry.entry_lo1.raw;

        cpu->cp0.entry_lo0.g = entry.entry_hi.g;
        cpu->cp0.entry_lo1.g = entry.entry_hi.g;
}
