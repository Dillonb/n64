#include "cache.h"
#include <system/n64system.h>
#include <mem/mem_util.h>

void writeback_dcache(u64 vaddr, u32 paddr) {
    int cache_line = get_dcache_line_index(vaddr);
    dcache_line_t* line = &N64CPU.dcache[cache_line];
    if (!line->valid) {
        logfatal("Writing back invalid dcache line");
    }
    u32 original_paddr = (line->ptag << 12) | (paddr & 0xFFF);
    u32 line_start = get_dcache_line_start(original_paddr);
    for (int i = 0; i < 16; i++) {
        n64sys.mem.rdram[line_start + i] = line->data[i];
    }
    line->dirty = false;
}

INLINE dcache_line_t* prep_dcache_line(u64 vaddr, u32 paddr) {
    int cache_line_index = get_dcache_line_index(vaddr);
    dcache_line_t* line = &N64CPU.dcache[cache_line_index];
    u32 ptag = get_paddr_ptag(paddr);

    bool valid = line->valid;
    bool ptag_matches = line->ptag == ptag;
    bool hit = valid && ptag_matches;
    bool dirty = line->dirty;

    // If the cache line is valid but dirty it contains data that is not written back to RAM yet.
    // If it's also not a hit, we are going to need to reload it next, so write it back.
    if (valid && dirty && !hit) {
        writeback_dcache(vaddr, paddr);
    }

    // If the cache line is not valid or it's not a hit, load it from RAM
    if (!valid || !hit) {
        u32 line_start = get_dcache_line_start(paddr);
        if (paddr < N64_RDRAM_SIZE) {
            for (int i = 0; i < 16; i++) {
                line->data[i] = n64sys.mem.rdram[line_start + i];
            }
        } else {
            logfatal("Implement me: Loading dcache from something other than RDRAM");
        }
        line->valid = true;
        line->dirty = false;
        line->ptag = ptag;
        line->index = cache_line_index;
    }

    return line;
}

u8 cache_read_byte(u64 vaddr, u32 paddr) {
    return prep_dcache_line(vaddr, paddr)->data[BYTE_ADDRESS(paddr & 0xF)];
}

void cache_write_byte(u64 vaddr, u32 paddr, u8 value) {
    dcache_line_t* line = prep_dcache_line(vaddr, paddr);
    line->data[BYTE_ADDRESS(paddr & 0xF)] = value;
    line->dirty = true;
}

u16 cache_read_half(u64 vaddr, u32 paddr) {
    return half_from_byte_array(prep_dcache_line(vaddr, paddr)->data, HALF_ADDRESS(paddr & 0xF));
}

void cache_write_half(u64 vaddr, u32 paddr, u16 value) {
    dcache_line_t* line = prep_dcache_line(vaddr, paddr);
    half_to_byte_array(line->data, HALF_ADDRESS(paddr & 0xF), value);
    line->dirty = true;
}

u32 cache_read_word(u64 vaddr, u32 paddr) {
    return word_from_byte_array(prep_dcache_line(vaddr, paddr)->data, WORD_ADDRESS(paddr & 0xF));
}

void cache_write_word(u64 vaddr, u32 paddr, u32 value) {
    dcache_line_t* line = prep_dcache_line(vaddr, paddr);
    word_to_byte_array(line->data, WORD_ADDRESS(paddr & 0xF), value);
    line->dirty = true;
}

u64 cache_read_dword(u64 vaddr, u32 paddr) {
    return dword_from_byte_array(prep_dcache_line(vaddr, paddr)->data, DWORD_ADDRESS(paddr & 0xF));
}

void cache_write_dword(u64 vaddr, u32 paddr, u64 value) {
    dcache_line_t* line = prep_dcache_line(vaddr, paddr);
    dword_to_byte_array(line->data, DWORD_ADDRESS(paddr & 0xF), value);
    line->dirty = true;
}
