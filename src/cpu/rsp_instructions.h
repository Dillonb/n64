#ifndef N64_RSP_INSTRUCTIONS_H
#define N64_RSP_INSTRUCTIONS_H
#include "rsp.h"
#include "mips_instruction_decode.h"
#define RSP_INSTR(NAME) void NAME(rsp_t* rsp, mips_instruction_t instruction)

RSP_INSTR(rsp_ori);
RSP_INSTR(rsp_addi);
RSP_INSTR(rsp_andi);

RSP_INSTR(rsp_lw);

RSP_INSTR(rsp_j);
RSP_INSTR(rsp_jal);

RSP_INSTR(rsp_beq);

RSP_INSTR(rsp_mfc0);
RSP_INSTR(rsp_mtc0);

#endif //N64_RSP_INSTRUCTIONS_H
