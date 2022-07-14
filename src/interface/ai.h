#ifndef N64_AI_H
#define N64_AI_H

#include <system/n64system.h>

void write_word_aireg(u32 address, u32 value);
u32 read_word_aireg(u32 address);
void ai_step(int cycles);

#endif //N64_AI_H
