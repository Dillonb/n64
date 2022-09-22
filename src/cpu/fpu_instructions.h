#ifndef N64_FPU_INSTRUCTIONS_H
#define N64_FPU_INSTRUCTIONS_H
#include "mips_instruction_decode.h"
#ifndef MIPS_INSTR
#define MIPS_INSTR(NAME) void NAME(mips_instruction_t instruction)
#endif

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

MIPS_INSTR(mips_cp_floor_w_d);
MIPS_INSTR(mips_cp_floor_w_s);

MIPS_INSTR(mips_cp_round_l_d);
MIPS_INSTR(mips_cp_round_l_s);
MIPS_INSTR(mips_cp_round_w_d);
MIPS_INSTR(mips_cp_round_w_s);

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

MIPS_INSTR(mips_cp_sqrt_s);
MIPS_INSTR(mips_cp_sqrt_d);

MIPS_INSTR(mips_cp_abs_s);
MIPS_INSTR(mips_cp_abs_d);

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
MIPS_INSTR(mips_cp_neg_s);
MIPS_INSTR(mips_cp_neg_d);

MIPS_INSTR(mips_mfc1);
MIPS_INSTR(mips_dmfc1);
MIPS_INSTR(mips_mtc1);
MIPS_INSTR(mips_dmtc1);

MIPS_INSTR(mips_cp1_invalid);
#endif //N64_FPU_INSTRUCTIONS_H
