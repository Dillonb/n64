#ifndef N64_RSP_INSTRUCTIONS_H
#define N64_RSP_INSTRUCTIONS_H
#include "rsp.h"
#include "mips_instruction_decode.h"
#define RSP_INSTR(NAME) void NAME(rsp_t* rsp, mips_instruction_t instruction)

RSP_INSTR(rsp_ori);
RSP_INSTR(rsp_addi);
RSP_INSTR(rsp_spc_add);
RSP_INSTR(rsp_andi);

RSP_INSTR(rsp_lw);

RSP_INSTR(rsp_j);
RSP_INSTR(rsp_jal);
RSP_INSTR(rsp_spc_jr);

RSP_INSTR(rsp_bne);
RSP_INSTR(rsp_beq);
RSP_INSTR(rsp_blez);

void rsp_mfc0(n64_system_t* system, mips_instruction_t instruction);
void rsp_mtc0(n64_system_t* system, mips_instruction_t instruction);

#endif //N64_RSP_INSTRUCTIONS_H
