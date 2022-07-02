#include <system/n64system.h>

#ifndef N64_VI_H
#define N64_VI_H

void write_word_vireg(word address, word value);
word read_word_vireg(word address);
void check_vi_interrupt();

#endif //N64_VI_H
