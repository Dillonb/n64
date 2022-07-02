#ifndef N64_AI_H
#define N64_AI_H

#include <system/n64system.h>

void write_word_aireg(word address, word value);
word read_word_aireg(word address);
void ai_step(int cycles);

#endif //N64_AI_H
