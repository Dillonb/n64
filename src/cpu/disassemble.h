#ifndef N64_DISASSEMBLE_H
#define N64_DISASSEMBLE_H

#include <util.h>

int disassemble(u32 address, u32 raw, char* buf, int buflen);
int disassemble_x86_64(uintptr_t address, u8* code, size_t code_size, char* buf, size_t buflen);

#endif //N64_DISASSEMBLE_H
