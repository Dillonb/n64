#ifndef N64_PI_H
#define N64_PI_H
#include <util.h>

word read_word_pireg(word address);
void write_word_pireg(word address, word value);
void on_pi_dma_complete();

#endif //N64_PI_H
