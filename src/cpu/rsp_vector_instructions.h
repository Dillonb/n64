#ifndef N64_RSP_VECTOR_INSTRUCTIONS_H
#define N64_RSP_VECTOR_INSTRUCTIONS_H

#include "mips_instruction_decode.h"
#include "rsp_types.h"

#define RSP_VECTOR_INSTR(NAME) void NAME(rsp_t* rsp, mips_instruction_t instruction)

RSP_VECTOR_INSTR(rsp_lwc2_lbv);
RSP_VECTOR_INSTR(rsp_lwc2_ldv);
RSP_VECTOR_INSTR(rsp_lwc2_lfv);
RSP_VECTOR_INSTR(rsp_lwc2_lhv);
RSP_VECTOR_INSTR(rsp_lwc2_llv);
RSP_VECTOR_INSTR(rsp_lwc2_lpv);
RSP_VECTOR_INSTR(rsp_lwc2_lqv);
RSP_VECTOR_INSTR(rsp_lwc2_lrv);
RSP_VECTOR_INSTR(rsp_lwc2_lsv);
RSP_VECTOR_INSTR(rsp_lwc2_ltv);
RSP_VECTOR_INSTR(rsp_lwc2_luv);

RSP_VECTOR_INSTR(rsp_swc2_sbv);
RSP_VECTOR_INSTR(rsp_swc2_sdv);
RSP_VECTOR_INSTR(rsp_swc2_sfv);
RSP_VECTOR_INSTR(rsp_swc2_shv);
RSP_VECTOR_INSTR(rsp_swc2_slv);
RSP_VECTOR_INSTR(rsp_swc2_spv);
RSP_VECTOR_INSTR(rsp_swc2_sqv);
RSP_VECTOR_INSTR(rsp_swc2_srv);
RSP_VECTOR_INSTR(rsp_swc2_ssv);
RSP_VECTOR_INSTR(rsp_swc2_stv);
RSP_VECTOR_INSTR(rsp_swc2_suv);

#endif //N64_RSP_VECTOR_INSTRUCTIONS_H
