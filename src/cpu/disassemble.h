#ifndef N64_DISASSEMBLE_H
#define N64_DISASSEMBLE_H

#include "../common/util.h"

int disassemble32(word address, word raw, char* buf, int buflen);

#endif //N64_DISASSEMBLE_H
