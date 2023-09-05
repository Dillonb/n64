#ifndef N64_RSP_INSTRUCTIONS_H
#define N64_RSP_INSTRUCTIONS_H
#include "rsp.h"
#include "mips_instruction_decode.h"
#define RSP_INSTR(NAME) void NAME(mips_instruction_t instruction)

RSP_INSTR(rsp_nop);

RSP_INSTR(rsp_ori);
RSP_INSTR(rsp_xori);
RSP_INSTR(rsp_lui);
RSP_INSTR(rsp_addi);
RSP_INSTR(rsp_spc_add);
RSP_INSTR(rsp_spc_and);
RSP_INSTR(rsp_andi);

RSP_INSTR(rsp_lbu);
RSP_INSTR(rsp_sb);
RSP_INSTR(rsp_sh);
RSP_INSTR(rsp_sw);
RSP_INSTR(rsp_lb);
RSP_INSTR(rsp_lhu);
RSP_INSTR(rsp_lh);
RSP_INSTR(rsp_lw);

RSP_INSTR(rsp_j);
RSP_INSTR(rsp_jal);
RSP_INSTR(rsp_slti);
RSP_INSTR(rsp_sltiu);
RSP_INSTR(rsp_spc_jr);
RSP_INSTR(rsp_spc_jalr);
RSP_INSTR(rsp_spc_sll);
RSP_INSTR(rsp_spc_srl);
RSP_INSTR(rsp_spc_sra);
RSP_INSTR(rsp_spc_srav);
RSP_INSTR(rsp_spc_srlv);
RSP_INSTR(rsp_spc_sllv);
RSP_INSTR(rsp_spc_sub);
RSP_INSTR(rsp_spc_or);
RSP_INSTR(rsp_spc_xor);
RSP_INSTR(rsp_spc_nor);
RSP_INSTR(rsp_spc_slt);
RSP_INSTR(rsp_spc_sltu);

void rsp_do_break();
RSP_INSTR(rsp_spc_break);

RSP_INSTR(rsp_bne);
RSP_INSTR(rsp_beq);
RSP_INSTR(rsp_bgtz);
RSP_INSTR(rsp_blez);

RSP_INSTR(rsp_ri_bltz);
RSP_INSTR(rsp_ri_bltzal);
RSP_INSTR(rsp_ri_bgez);
RSP_INSTR(rsp_ri_bgezal);

RSP_INSTR(rsp_mfc0);
RSP_INSTR(rsp_mtc0);

#endif //N64_RSP_INSTRUCTIONS_H
