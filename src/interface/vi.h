#include <system/n64system.h>

#ifndef N64_VI_H
#define N64_VI_H

void write_word_vireg(u32 address, u32 value);
u32 read_word_vireg(u32 address);
void check_vi_interrupt();

#endif //N64_VI_H
