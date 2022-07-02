#ifndef N64_TLB_INSTRUCTIONS_H
#define N64_TLB_INSTRUCTIONS_H

#include "r4300i.h"
#include "mips_instruction_decode.h"

#ifndef MIPS_INSTR
#define MIPS_INSTR(NAME) void NAME(mips_instruction_t instruction)
#endif

MIPS_INSTR(mips_tlbwi);
MIPS_INSTR(mips_tlbp);
MIPS_INSTR(mips_tlbr);
MIPS_INSTR(mips_tlbwr);

#endif //N64_TLB_INSTRUCTIONS_H
