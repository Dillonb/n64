#ifndef N64_MIPS32_H
#define N64_MIPS32_H
#include "r4300i.h"

#define MIPS32_INSTR(NAME) void NAME(r4300i_t* cpu, mips32_instruction_t instruction)

MIPS32_INSTR(mips32_addi);
MIPS32_INSTR(mips32_addiu);
MIPS32_INSTR(mips32_addu);

MIPS32_INSTR(mips32_and);
MIPS32_INSTR(mips32_andi);

MIPS32_INSTR(mips32_blezl);
MIPS32_INSTR(mips32_beq);
MIPS32_INSTR(mips32_beql);
MIPS32_INSTR(mips32_bgtz);
MIPS32_INSTR(mips32_bne);
MIPS32_INSTR(mips32_bnel);

MIPS32_INSTR(mips32_cache);

MIPS32_INSTR(mips32_j);
MIPS32_INSTR(mips32_jal);

MIPS32_INSTR(mips32_slti);

MIPS32_INSTR(mips32_mtc0);
MIPS32_INSTR(mips32_lui);
MIPS32_INSTR(mips32_lbu);
MIPS32_INSTR(mips32_lw);
MIPS32_INSTR(mips32_sb);
MIPS32_INSTR(mips32_sw);
MIPS32_INSTR(mips32_ori);

MIPS32_INSTR(mips32_xori);

MIPS32_INSTR(mips32_lb);

MIPS32_INSTR(mips32_spc_srl);
MIPS32_INSTR(mips32_spc_sllv);
MIPS32_INSTR(mips32_spc_srlv);
MIPS32_INSTR(mips32_spc_jr);
MIPS32_INSTR(mips32_spc_mfhi);
MIPS32_INSTR(mips32_spc_mflo);
MIPS32_INSTR(mips32_spc_multu);
MIPS32_INSTR(mips32_spc_add);
MIPS32_INSTR(mips32_spc_addu);
MIPS32_INSTR(mips32_spc_and);
MIPS32_INSTR(mips32_spc_subu);
MIPS32_INSTR(mips32_spc_or);
MIPS32_INSTR(mips32_spc_xor);
MIPS32_INSTR(mips32_spc_slt);
MIPS32_INSTR(mips32_spc_sltu);


MIPS32_INSTR(mips32_ri_bgezl);
MIPS32_INSTR(mips32_ri_bgezal);

#endif //N64_MIPS32_H
