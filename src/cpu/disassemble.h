#ifndef N64_DISASSEMBLE_H
#define N64_DISASSEMBLE_H

#include <util.h>

int disassemble(u32 address, u32 raw, char* buf, int buflen);

#endif //N64_DISASSEMBLE_H
