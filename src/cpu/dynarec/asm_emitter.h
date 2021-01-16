#ifndef N64_ASM_EMITTER_H
#define N64_ASM_EMITTER_H

#include "dynarec.h"
#include <system/n64system.h>
#include <dynasm/dasm_proto.h>

#define COMPILER(name) void compile_##name(dasm_State** Dst, mips_instruction_t instr, word address, word* extra_cycles)

COMPILER(mips_addiu);
COMPILER(mips_beq);
COMPILER(mips_cache);
COMPILER(mips_daddi);
COMPILER(mips_daddiu);
COMPILER(mips_andi);
COMPILER(mips_ori);
COMPILER(mips_slti);
COMPILER(mips_sltiu);
COMPILER(mips_xori);
COMPILER(mips_spc_sll);
COMPILER(mips_spc_srl);
COMPILER(mips_spc_sra);
COMPILER(mips_spc_srav);
COMPILER(mips_spc_sllv);
COMPILER(mips_spc_srlv);
COMPILER(mips_spc_mfhi);
COMPILER(mips_spc_mthi);
COMPILER(mips_spc_mflo);
COMPILER(mips_spc_mtlo);
COMPILER(mips_spc_dsllv);
COMPILER(mips_spc_dsrlv);
COMPILER(mips_spc_mult);
COMPILER(mips_spc_multu);
COMPILER(mips_spc_div);
COMPILER(mips_spc_divu);
COMPILER(mips_spc_dmult);
COMPILER(mips_spc_dmultu);
COMPILER(mips_spc_ddiv);
COMPILER(mips_spc_ddivu);
COMPILER(mips_spc_add);
COMPILER(mips_spc_addu);
COMPILER(mips_spc_and);
COMPILER(mips_spc_nor);
COMPILER(mips_spc_sub);
COMPILER(mips_spc_subu);
COMPILER(mips_spc_or);
COMPILER(mips_spc_xor);
COMPILER(mips_spc_slt);
COMPILER(mips_spc_sltu);
COMPILER(mips_spc_dadd);
COMPILER(mips_spc_daddu);
COMPILER(mips_spc_dsubu);
COMPILER(mips_spc_teq); // TODO this needs to be marked as TRAP and special logic added to the emitted code
COMPILER(mips_spc_dsll);
COMPILER(mips_spc_dsrl);
COMPILER(mips_spc_dsra);
COMPILER(mips_spc_dsll32);
COMPILER(mips_spc_dsrl32);
COMPILER(mips_spc_dsra32);

// Load-stores
COMPILER(mips_lbu);
COMPILER(mips_lhu);
COMPILER(mips_lh);
COMPILER(mips_lw);
COMPILER(mips_lwu);
COMPILER(mips_sb);
COMPILER(mips_sh);
COMPILER(mips_sw);
COMPILER(mips_sd);
COMPILER(mips_lb);
COMPILER(mips_lui);
COMPILER(mips_ld);
COMPILER(mips_ldc1);
COMPILER(mips_sdc1);
COMPILER(mips_lwc1);
COMPILER(mips_swc1);
COMPILER(mips_lwl);
COMPILER(mips_lwr);
COMPILER(mips_swl);
COMPILER(mips_swr);
COMPILER(mips_ldl);
COMPILER(mips_ldr);
COMPILER(mips_sdl);
COMPILER(mips_sdr);

// Unoptimized branches
COMPILER(mips_beql);
COMPILER(mips_bgtz);
COMPILER(mips_bgtzl);
COMPILER(mips_blez);
COMPILER(mips_blezl);
COMPILER(mips_bne);
COMPILER(mips_bnel);
COMPILER(mips_j);
COMPILER(mips_jal);
COMPILER(mips_ri_bltz);
COMPILER(mips_ri_bltzl);
COMPILER(mips_ri_bltzal);
COMPILER(mips_ri_bgez);
COMPILER(mips_ri_bgezl);
COMPILER(mips_ri_bgezal);
COMPILER(mips_spc_jr);
COMPILER(mips_spc_jalr);
COMPILER(mips_cp_bc1tl);
COMPILER(mips_cp_bc1fl);

// Instructions that don't make too much sense to optimize
COMPILER(mips_mfc0);
COMPILER(mips_mtc0);
COMPILER(mips_tlbwi);
COMPILER(mips_tlbp);
COMPILER(mips_tlbr);
COMPILER(mips_eret);


// CP1 stuff
COMPILER(mips_cfc1);
COMPILER(mips_mfc1);
COMPILER(mips_dmfc1);
COMPILER(mips_mtc1);
COMPILER(mips_dmtc1);
COMPILER(mips_ctc1);
COMPILER(mips_cp_bc1t);
COMPILER(mips_cp_bc1f);
COMPILER(mips_cp_add_d);
COMPILER(mips_cp_add_s);
COMPILER(mips_cp_sub_d);
COMPILER(mips_cp_sub_s);
COMPILER(mips_cp_mul_d);
COMPILER(mips_cp_mul_s);
COMPILER(mips_cp_div_d);
COMPILER(mips_cp_div_s);
COMPILER(mips_cp_trunc_l_d);
COMPILER(mips_cp_trunc_l_s);
COMPILER(mips_cp_trunc_w_d);
COMPILER(mips_cp_trunc_w_s);
COMPILER(mips_cp_cvt_d_s);
COMPILER(mips_cp_cvt_d_w);
COMPILER(mips_cp_cvt_d_l);
COMPILER(mips_cp_cvt_l_d);
COMPILER(mips_cp_cvt_l_s);
COMPILER(mips_cp_cvt_s_d);
COMPILER(mips_cp_cvt_s_w);
COMPILER(mips_cp_cvt_s_l);
COMPILER(mips_cp_cvt_w_d);
COMPILER(mips_cp_cvt_w_s);
COMPILER(mips_cp_sqrt_d);
COMPILER(mips_cp_sqrt_s);
COMPILER(mips_cp_abs_d);
COMPILER(mips_cp_abs_s);
COMPILER(mips_cp_mov_d);
COMPILER(mips_cp_mov_s);
COMPILER(mips_cp_neg_d);
COMPILER(mips_cp_neg_s);
COMPILER(mips_cp_c_eq_d);
COMPILER(mips_cp_c_eq_s);
COMPILER(mips_cp_c_lt_d);
COMPILER(mips_cp_c_lt_s);
COMPILER(mips_cp_c_le_d);
COMPILER(mips_cp_c_le_s);

dasm_State* block_header();
void clear_branch_flag(dasm_State** Dst);
void advance_pc(r4300i_t* compile_time_cpu, dasm_State** Dst);
dynarec_ir_t* instruction_ir(mips_instruction_t instr, word address);
void end_block(dasm_State** Dst, int block_length);
void post_branch_likely(dasm_State** Dst, r4300i_t* compile_time_cpu, int block_length);
void check_exception(dasm_State** Dst, word block_length);
void flush_prev_pc(dasm_State** Dst, dword prev_pc);
void flush_pc(dasm_State** Dst, dword pc);
void flush_next_pc(dasm_State** Dst, dword next_pc);
#endif //N64_ASM_EMITTER_H
