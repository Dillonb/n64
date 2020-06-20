#ifndef N64_MIPS_H
#define N64_MIPS_H
#include "r4300i.h"

#define MIPS_INSTR(NAME) void NAME(r4300i_t* cpu, mips_instruction_t instruction)

MIPS_INSTR(mips_addi);
MIPS_INSTR(mips_addiu);
MIPS_INSTR(mips_addu);

MIPS_INSTR(mips_daddi);

MIPS_INSTR(mips_and);
MIPS_INSTR(mips_andi);

MIPS_INSTR(mips_blez);
MIPS_INSTR(mips_blezl);
MIPS_INSTR(mips_beq);
MIPS_INSTR(mips_beql);
MIPS_INSTR(mips_bgtz);
MIPS_INSTR(mips_bne);
MIPS_INSTR(mips_bnel);

MIPS_INSTR(mips_cache);

MIPS_INSTR(mips_j);
MIPS_INSTR(mips_jal);

MIPS_INSTR(mips_slti);
MIPS_INSTR(mips_sltiu);

MIPS_INSTR(mips_mfc0);
MIPS_INSTR(mips_mtc0);

MIPS_INSTR(mips_cfc1);
MIPS_INSTR(mips_ctc1);

MIPS_INSTR(mips_cp_mul_d);
MIPS_INSTR(mips_cp_mul_s);

MIPS_INSTR(mips_ld);
MIPS_INSTR(mips_lui);
MIPS_INSTR(mips_lbu);
MIPS_INSTR(mips_lhu);
MIPS_INSTR(mips_lw);
MIPS_INSTR(mips_sb);
MIPS_INSTR(mips_sh);
MIPS_INSTR(mips_sd);
MIPS_INSTR(mips_sw);
MIPS_INSTR(mips_ori);

MIPS_INSTR(mips_xori);

MIPS_INSTR(mips_lb);

MIPS_INSTR(mips_ldc1);
MIPS_INSTR(mips_sdc1);
MIPS_INSTR(mips_lwc1);
MIPS_INSTR(mips_swc1);

MIPS_INSTR(mips_spc_sll);
MIPS_INSTR(mips_spc_srl);
MIPS_INSTR(mips_spc_sra);
MIPS_INSTR(mips_spc_sllv);
MIPS_INSTR(mips_spc_srlv);
MIPS_INSTR(mips_spc_jr);
MIPS_INSTR(mips_spc_mfhi);
MIPS_INSTR(mips_spc_mthi);
MIPS_INSTR(mips_spc_mflo);
MIPS_INSTR(mips_spc_mtlo);
MIPS_INSTR(mips_spc_mult);
MIPS_INSTR(mips_spc_multu);
MIPS_INSTR(mips_spc_divu);
MIPS_INSTR(mips_spc_add);
MIPS_INSTR(mips_spc_addu);
MIPS_INSTR(mips_spc_and);
MIPS_INSTR(mips_spc_subu);
MIPS_INSTR(mips_spc_or);
MIPS_INSTR(mips_spc_xor);
MIPS_INSTR(mips_spc_slt);
MIPS_INSTR(mips_spc_sltu);
MIPS_INSTR(mips_spc_dadd);


MIPS_INSTR(mips_ri_bgez);
MIPS_INSTR(mips_ri_bgezl);
MIPS_INSTR(mips_ri_bgezal);

#endif //N64_MIPS_H
