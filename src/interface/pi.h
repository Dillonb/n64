#ifndef N64_PI_H
#define N64_PI_H
#include <util.h>

u32 read_word_pireg(u32 address);
void write_word_pireg(u32 address, u32 value);
void on_pi_dma_complete();

#endif //N64_PI_H
