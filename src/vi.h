#include "system/n64system.h"

#ifndef N64_VI_H
#define N64_VI_H

void write_word_vireg(n64_system_t* system, word address, word value);
word read_word_vireg(n64_system_t* system, word address);
void check_vi_interrupt(n64_system_t* system);

#endif //N64_VI_H
