#ifndef N64_MIPS32_H
#define N64_MIPS32_H
#include "r4300i.h"

#define MIPS32_INSTR(NAME) void NAME(r4300i_t* cpu, mips32_instruction_t instruction)

MIPS32_INSTR(addi);
MIPS32_INSTR(addiu);
MIPS32_INSTR(addu);

MIPS32_INSTR(and);
MIPS32_INSTR(andi);

MIPS32_INSTR(blezl);
MIPS32_INSTR(beq);
MIPS32_INSTR(beql);
MIPS32_INSTR(bne);
MIPS32_INSTR(bnel);

MIPS32_INSTR(cache);

MIPS32_INSTR(jal);

MIPS32_INSTR(slti);

MIPS32_INSTR(mtc0);
MIPS32_INSTR(lui);
MIPS32_INSTR(lbu);
MIPS32_INSTR(lw);
MIPS32_INSTR(sb);
MIPS32_INSTR(sw);
MIPS32_INSTR(ori);

MIPS32_INSTR(xori);

MIPS32_INSTR(spc_srl);
MIPS32_INSTR(spc_jr);
MIPS32_INSTR(spc_mfhi);
MIPS32_INSTR(spc_mflo);
MIPS32_INSTR(spc_multu);
MIPS32_INSTR(spc_add);
MIPS32_INSTR(spc_addu);
MIPS32_INSTR(spc_and);
MIPS32_INSTR(spc_subu);
MIPS32_INSTR(spc_or);
MIPS32_INSTR(spc_slt);
MIPS32_INSTR(spc_sltu);


MIPS32_INSTR(ri_bgezl);

#endif //N64_MIPS32_H
