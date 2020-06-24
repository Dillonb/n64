#ifndef N64_AI_H
#define N64_AI_H

#include "../system/n64system.h"

void write_word_aireg(n64_system_t* system, word address, word value);
word read_word_aireg(n64_system_t* system, word address);
void ai_step(n64_system_t* system, int cycles);

#endif //N64_AI_H
