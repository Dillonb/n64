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
MIPS_INSTR(mips_mfc1);
MIPS_INSTR(mips_mtc1);

MIPS_INSTR(mips_eret);

MIPS_INSTR(mips_cfc1);
MIPS_INSTR(mips_ctc1);

MIPS_INSTR(mips_cp_bc1f);
MIPS_INSTR(mips_cp_bc1fl);
MIPS_INSTR(mips_cp_bc1t);
MIPS_INSTR(mips_cp_bc1tl);

MIPS_INSTR(mips_cp_mul_d);
MIPS_INSTR(mips_cp_mul_s);
MIPS_INSTR(mips_cp_div_d);
MIPS_INSTR(mips_cp_div_s);
MIPS_INSTR(mips_cp_add_d);
MIPS_INSTR(mips_cp_add_s);
MIPS_INSTR(mips_cp_sub_d);
MIPS_INSTR(mips_cp_sub_s);

MIPS_INSTR(mips_cp_trunc_l_d);
MIPS_INSTR(mips_cp_trunc_l_s);
MIPS_INSTR(mips_cp_trunc_w_d);
MIPS_INSTR(mips_cp_trunc_w_s);

MIPS_INSTR(mips_cp_cvt_d_s);
MIPS_INSTR(mips_cp_cvt_d_w);
MIPS_INSTR(mips_cp_cvt_d_l);

MIPS_INSTR(mips_cp_cvt_l_s);
MIPS_INSTR(mips_cp_cvt_l_d);

MIPS_INSTR(mips_cp_cvt_s_d);
MIPS_INSTR(mips_cp_cvt_s_w);
MIPS_INSTR(mips_cp_cvt_s_l);

MIPS_INSTR(mips_cp_cvt_w_s);
MIPS_INSTR(mips_cp_cvt_w_d);

MIPS_INSTR(mips_cp_c_f_s);
MIPS_INSTR(mips_cp_c_un_s);
MIPS_INSTR(mips_cp_c_eq_s);
MIPS_INSTR(mips_cp_c_ueq_s);
MIPS_INSTR(mips_cp_c_olt_s);
MIPS_INSTR(mips_cp_c_ult_s);
MIPS_INSTR(mips_cp_c_ole_s);
MIPS_INSTR(mips_cp_c_ule_s);
MIPS_INSTR(mips_cp_c_sf_s);
MIPS_INSTR(mips_cp_c_ngle_s);
MIPS_INSTR(mips_cp_c_seq_s);
MIPS_INSTR(mips_cp_c_ngl_s);
MIPS_INSTR(mips_cp_c_lt_s);
MIPS_INSTR(mips_cp_c_nge_s);
MIPS_INSTR(mips_cp_c_le_s);
MIPS_INSTR(mips_cp_c_ngt_s);
MIPS_INSTR(mips_cp_c_f_d);
MIPS_INSTR(mips_cp_c_un_d);
MIPS_INSTR(mips_cp_c_eq_d);
MIPS_INSTR(mips_cp_c_ueq_d);
MIPS_INSTR(mips_cp_c_olt_d);
MIPS_INSTR(mips_cp_c_ult_d);
MIPS_INSTR(mips_cp_c_ole_d);
MIPS_INSTR(mips_cp_c_ule_d);
MIPS_INSTR(mips_cp_c_sf_d);
MIPS_INSTR(mips_cp_c_ngle_d);
MIPS_INSTR(mips_cp_c_seq_d);
MIPS_INSTR(mips_cp_c_ngl_d);
MIPS_INSTR(mips_cp_c_lt_d);
MIPS_INSTR(mips_cp_c_nge_d);
MIPS_INSTR(mips_cp_c_le_d);
MIPS_INSTR(mips_cp_c_ngt_d);

MIPS_INSTR(mips_cp_mov_s);
MIPS_INSTR(mips_cp_mov_d);

MIPS_INSTR(mips_ld);
MIPS_INSTR(mips_lui);
MIPS_INSTR(mips_lbu);
MIPS_INSTR(mips_lhu);
MIPS_INSTR(mips_lh);
MIPS_INSTR(mips_lw);
MIPS_INSTR(mips_lwu);
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
MIPS_INSTR(mips_lwl);
MIPS_INSTR(mips_lwr);
MIPS_INSTR(mips_swl);
MIPS_INSTR(mips_swr);
MIPS_INSTR(mips_ldl);
MIPS_INSTR(mips_ldr);
MIPS_INSTR(mips_sdl);
MIPS_INSTR(mips_sdr);

MIPS_INSTR(mips_spc_sll);
MIPS_INSTR(mips_spc_srl);
MIPS_INSTR(mips_spc_sra);
MIPS_INSTR(mips_spc_srav);
MIPS_INSTR(mips_spc_sllv);
MIPS_INSTR(mips_spc_srlv);
MIPS_INSTR(mips_spc_jr);
MIPS_INSTR(mips_spc_jalr);
MIPS_INSTR(mips_spc_mfhi);
MIPS_INSTR(mips_spc_mthi);
MIPS_INSTR(mips_spc_mflo);
MIPS_INSTR(mips_spc_mtlo);
MIPS_INSTR(mips_spc_mult);
MIPS_INSTR(mips_spc_multu);
MIPS_INSTR(mips_spc_div);
MIPS_INSTR(mips_spc_divu);
MIPS_INSTR(mips_spc_add);
MIPS_INSTR(mips_spc_addu);
MIPS_INSTR(mips_spc_nor);
MIPS_INSTR(mips_spc_and);
MIPS_INSTR(mips_spc_subu);
MIPS_INSTR(mips_spc_or);
MIPS_INSTR(mips_spc_xor);
MIPS_INSTR(mips_spc_slt);
MIPS_INSTR(mips_spc_sltu);
MIPS_INSTR(mips_spc_dadd);
MIPS_INSTR(mips_spc_dsll);
MIPS_INSTR(mips_spc_dsll32);


MIPS_INSTR(mips_ri_bltz);
MIPS_INSTR(mips_ri_bltzl);
MIPS_INSTR(mips_ri_bgez);
MIPS_INSTR(mips_ri_bgezl);
MIPS_INSTR(mips_ri_bgezal);

#endif //N64_MIPS_H
