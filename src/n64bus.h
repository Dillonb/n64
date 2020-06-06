#ifndef N64_N64BUS_H
#define N64_N64BUS_H

#include "common/util.h"

void n64_write_word(word address, word value);
word n64_read_word(word address);

void n64_write_byte(word address, byte value);
byte n64_read_byte(word address);

#endif //N64_N64BUS_H
