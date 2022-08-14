#ifndef N64_RSP_VECTOR_INSTRUCTIONS_H
#define N64_RSP_VECTOR_INSTRUCTIONS_H

#include "mips_instruction_decode.h"
#include "rsp_types.h"

#define RSP_VECTOR_INSTR(NAME) void NAME(mips_instruction_t instruction)

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
RSP_VECTOR_INSTR(rsp_swc2_swv);

RSP_VECTOR_INSTR(rsp_cfc2);
RSP_VECTOR_INSTR(rsp_ctc2);
RSP_VECTOR_INSTR(rsp_mfc2);
RSP_VECTOR_INSTR(rsp_mtc2);

RSP_VECTOR_INSTR(rsp_vec_vabs);
RSP_VECTOR_INSTR(rsp_vec_vadd);
RSP_VECTOR_INSTR(rsp_vec_vaddc);
RSP_VECTOR_INSTR(rsp_vec_vand);
RSP_VECTOR_INSTR(rsp_vec_vch);
RSP_VECTOR_INSTR(rsp_vec_vcl);
RSP_VECTOR_INSTR(rsp_vec_vcr);
RSP_VECTOR_INSTR(rsp_vec_veq);
RSP_VECTOR_INSTR(rsp_vec_vge);
RSP_VECTOR_INSTR(rsp_vec_vlt);
RSP_VECTOR_INSTR(rsp_vec_vmacf);
RSP_VECTOR_INSTR(rsp_vec_vmacq);
RSP_VECTOR_INSTR(rsp_vec_vmacu);
RSP_VECTOR_INSTR(rsp_vec_vmadh);
RSP_VECTOR_INSTR(rsp_vec_vmadl);
RSP_VECTOR_INSTR(rsp_vec_vmadm);
RSP_VECTOR_INSTR(rsp_vec_vmadn);
RSP_VECTOR_INSTR(rsp_vec_vmov);
RSP_VECTOR_INSTR(rsp_vec_vmrg);
RSP_VECTOR_INSTR(rsp_vec_vmudh);
RSP_VECTOR_INSTR(rsp_vec_vmudl);
RSP_VECTOR_INSTR(rsp_vec_vmudm);
RSP_VECTOR_INSTR(rsp_vec_vmudn);
RSP_VECTOR_INSTR(rsp_vec_vmulf);
RSP_VECTOR_INSTR(rsp_vec_vmulq);
RSP_VECTOR_INSTR(rsp_vec_vmulu);
RSP_VECTOR_INSTR(rsp_vec_vnand);
RSP_VECTOR_INSTR(rsp_vec_vne);
RSP_VECTOR_INSTR(rsp_vec_vnop);
RSP_VECTOR_INSTR(rsp_vec_vnor);
RSP_VECTOR_INSTR(rsp_vec_vnxor);
RSP_VECTOR_INSTR(rsp_vec_vor);
RSP_VECTOR_INSTR(rsp_vec_vrcp);
RSP_VECTOR_INSTR(rsp_vec_vrcph_vrsqh);
RSP_VECTOR_INSTR(rsp_vec_vrcpl);
RSP_VECTOR_INSTR(rsp_vec_vrndn);
RSP_VECTOR_INSTR(rsp_vec_vrndp);
RSP_VECTOR_INSTR(rsp_vec_vrsq);
RSP_VECTOR_INSTR(rsp_vec_vrsql);
RSP_VECTOR_INSTR(rsp_vec_vsar);
RSP_VECTOR_INSTR(rsp_vec_vsub);
RSP_VECTOR_INSTR(rsp_vec_vsubc);
RSP_VECTOR_INSTR(rsp_vec_vxor);
RSP_VECTOR_INSTR(rsp_vec_vzero);

#endif //N64_RSP_VECTOR_INSTRUCTIONS_H
