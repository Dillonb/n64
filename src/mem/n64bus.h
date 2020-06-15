#ifndef N64_N64BUS_H
#define N64_N64BUS_H

#include "../common/util.h"
#include "../system/n64system.h"

void n64_write_word(n64_system_t* system, word address, word value);
word n64_read_word(n64_system_t* system, word address);

void n64_write_byte(n64_system_t* system, word address, byte value);
byte n64_read_byte(n64_system_t* system, word address);

#endif //N64_N64BUS_H
