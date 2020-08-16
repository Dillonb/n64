#ifndef N64_DISASSEMBLE_H
#define N64_DISASSEMBLE_H

#include <util.h>

int disassemble(word address, word raw, char* buf, int buflen);

#endif //N64_DISASSEMBLE_H
