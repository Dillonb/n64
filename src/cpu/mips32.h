#ifndef N64_MIPS32_H
#define N64_MIPS32_H
#include "r4300i.h"

#define MIPS32_INSTR(NAME) void NAME(r4300i_t* cpu, mips32_instruction_t instruction)

MIPS32_INSTR(add);
MIPS32_INSTR(addi);
MIPS32_INSTR(addiu);
MIPS32_INSTR(addu);

MIPS32_INSTR(and);
MIPS32_INSTR(andi);

MIPS32_INSTR(beq);
MIPS32_INSTR(beql);
MIPS32_INSTR(bne);

MIPS32_INSTR(jal);

MIPS32_INSTR(slti);

MIPS32_INSTR(mtc0);
MIPS32_INSTR(lui);
MIPS32_INSTR(lw);
MIPS32_INSTR(sw);
MIPS32_INSTR(ori);

MIPS32_INSTR(xori);

#endif //N64_MIPS32_H
