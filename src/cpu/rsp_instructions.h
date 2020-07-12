#ifndef N64_RSP_INSTRUCTIONS_H
#define N64_RSP_INSTRUCTIONS_H
#include "rsp.h"
#include "mips_instruction_decode.h"
#define RSP_INSTR(NAME) void NAME(rsp_t* rsp, mips_instruction_t instruction)

RSP_INSTR(rsp_ori);
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
RSP_INSTR(rsp_spc_jr);
RSP_INSTR(rsp_spc_sll);
RSP_INSTR(rsp_spc_srl);
RSP_INSTR(rsp_spc_sra);
RSP_INSTR(rsp_spc_sllv);
RSP_INSTR(rsp_spc_sub);
RSP_INSTR(rsp_spc_or);
RSP_INSTR(rsp_spc_xor);

void rsp_spc_break(n64_system_t* system, mips_instruction_t instruction);

RSP_INSTR(rsp_bne);
RSP_INSTR(rsp_beq);
RSP_INSTR(rsp_bgtz);
RSP_INSTR(rsp_blez);

RSP_INSTR(rsp_ri_bltz);
RSP_INSTR(rsp_ri_bgez);
RSP_INSTR(rsp_ri_bgezal);

void rsp_mfc0(n64_system_t* system, mips_instruction_t instruction);
void rsp_mtc0(n64_system_t* system, mips_instruction_t instruction);

#endif //N64_RSP_INSTRUCTIONS_H
