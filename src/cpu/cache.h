#ifndef N64_CACHE_H
#define N64_CACHE_H

// #include <mem/n64bus.h>
#include <util.h>

typedef struct icache_line {
    bool valid;
    u32 ptag;
    // since the icache will only ever be accessed as u32, just store it as u32
    u32 data[8]; // 32 bytes, 8 words
} icache_line_t;

typedef struct dcache_line {
    bool valid;
    bool dirty;
    u32 ptag;
    u8 data[16];
    int index;
} dcache_line_t;

INLINE int get_icache_line_index(u64 vaddr) {
    return (vaddr >> 5) & 0x1FF;
}

INLINE u32 get_icache_line_start(u32 paddr) {
    return paddr & ~0x1F;
}

INLINE int get_dcache_line_index(u64 vaddr) {
    return (vaddr >> 4) & 0x1FF;
}

INLINE u32 get_dcache_line_start(u32 paddr) {
    return paddr & ~0xF;
}

INLINE u32 get_paddr_ptag(u32 paddr) {
    return paddr >> 12;
}

void writeback_dcache(u64 vaddr, u32 paddr);

u8 cache_read_byte(u64 vaddr, u32 paddr);
void cache_write_byte(u64 vaddr, u32 paddr, u8 value);
u16 cache_read_half(u64 vaddr, u32 paddr);
void cache_write_half(u64 vaddr, u32 paddr, u16 value);
u32 cache_read_word(u64 vaddr, u32 paddr);
void cache_write_word(u64 vaddr, u32 paddr, u32 value);
u64 cache_read_dword(u64 vaddr, u32 paddr);
void cache_write_dword(u64 vaddr, u32 paddr, u64 value);


#endif // N64_CACHE_H