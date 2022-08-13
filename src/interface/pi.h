#ifndef N64_PI_H
#define N64_PI_H
#include <util.h>

u32 read_word_pireg(u32 address);
void write_word_pireg(u32 address, u32 value);

void write_byte_pibus(u32 address, u32 value);
u8 read_byte_pibus(u32 address);

void write_half_pibus(u32 address, u16 value);
u16 read_half_pibus(u32 address);

void write_word_pibus(u32 address, u32 value);
u32 read_word_pibus(u32 address);

void write_dword_pibus(u32 address, u64 value);
u64 read_dword_pibus(u32 address);

void on_pi_dma_complete();
void on_pi_write_complete();

#endif //N64_PI_H
