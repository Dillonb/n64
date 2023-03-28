#ifndef N64_V2_EMITTER_H
#define N64_V2_EMITTER_H

#include <dynasm/dasm_proto.h>
#include "ir_context.h"
#include <cpu/dynarec/dynarec.h>

dasm_State** v2_block_header();
dasm_State** v2_emit_run_block();
void v2_dasm_free();

enum args_reversed {
    ARGS_NORMAL_ORDER = 0,
    ARGS_REVERSED = 1 // in this order so `if (args_reversed)` is valid
};

void host_emit_mov_reg_imm(dasm_State** Dst, ir_register_allocation_t reg_alloc, ir_set_constant_t imm_value);
void host_emit_mov_reg_reg(dasm_State** Dst, ir_register_allocation_t dst_reg_alloc, ir_register_allocation_t src_reg_alloc, ir_value_type_t source_value_type);

void host_emit_and_reg_imm(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_set_constant_t operand2);
void host_emit_and_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc);
void host_emit_or_reg_imm(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_set_constant_t operand2);
void host_emit_or_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc);
void host_emit_xor_reg_imm(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_set_constant_t operand2);
void host_emit_xor_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc);
void host_emit_add_reg_imm(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_set_constant_t operand2);
void host_emit_add_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc);
void host_emit_sub_reg_reg(dasm_State** Dst, ir_register_allocation_t minuend_alloc, ir_register_allocation_t subtrahend_alloc);
void host_emit_sub_reg_imm(dasm_State** Dst, ir_register_allocation_t minuend_alloc, ir_set_constant_t subtrahend);
void host_emit_shift_reg_imm(dasm_State** Dst, ir_register_allocation_t reg_alloc, ir_value_type_t type, u8 shift_amount, ir_shift_direction_t direction);
void host_emit_shift_reg_reg(dasm_State** Dst, ir_register_allocation_t reg_alloc, ir_value_type_t type, ir_register_allocation_t amount_reg_alloc, ir_shift_direction_t direction);
void host_emit_bitwise_not(dasm_State** Dst, ir_register_allocation_t reg_alloc);
void host_emit_mult_reg_imm(dasm_State** Dst, ir_register_allocation_t reg_alloc, ir_set_constant_t imm, ir_value_type_t multiplicand_type);
void host_emit_mult_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc, ir_value_type_t multiplicand_type);
void host_emit_mult_imm_imm(dasm_State** Dst, ir_set_constant_t operand1, ir_set_constant_t operand2, ir_value_type_t multiplicand_type);
void host_emit_div_reg_imm(dasm_State** Dst, ir_register_allocation_t reg_alloc, ir_set_constant_t imm, ir_value_type_t divide_type);
void host_emit_div_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc, ir_value_type_t divide_type);

void v2_end_block(dasm_State** Dst, int block_length);
void host_emit_cmp_reg_imm(dasm_State** Dst, ir_register_allocation_t dest_reg_alloc, ir_condition_t cond, ir_register_allocation_t operand1_alloc, ir_set_constant_t operand2, enum args_reversed args_reversed);
void host_emit_cmp_reg_reg(dasm_State** Dst, ir_register_allocation_t dest_reg_alloc, ir_condition_t cond, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc, enum args_reversed args_reversed);
void host_emit_cmov_pc_binary(dasm_State** Dst, ir_register_allocation_t cond_register_alloc, ir_instruction_t* if_true, ir_instruction_t* if_false);
void host_emit_mov_pc(dasm_State** Dst, ir_instruction_t* value);
void host_emit_mov_mem_imm(dasm_State** Dst, uintptr_t mem, ir_set_constant_t value, ir_value_type_t write_size);
void host_emit_mov_mem_reg(dasm_State** Dst, uintptr_t mem, ir_register_allocation_t reg_alloc, ir_value_type_t type);
void host_emit_mov_reg_mem(dasm_State** Dst, ir_register_allocation_t reg_alloc, uintptr_t mem, ir_value_type_t type);
void host_emit_mov_reg_cp0(dasm_State** Dst, ir_register_allocation_t reg_alloc, int cp0_reg);
void host_emit_mov_cp0_imm(dasm_State** Dst, int cp0_reg, ir_set_constant_t value);
void host_emit_mov_cp0_reg(dasm_State** Dst, int cp0_reg, ir_register_allocation_t reg_alloc);
void host_emit_ret(dasm_State** Dst, ir_instruction_flush_t* flush_iter, int block_length);
void host_emit_exception_to_args(dasm_State** Dst, dynarec_exception_t exception);
void host_emit_cond_ret(dasm_State** Dst, ir_register_allocation_t cond_reg_alloc, ir_instruction_flush_t* flush_iter, int block_length, bool has_exception, dynarec_exception_t exception);

void host_emit_mov_fgr_gpr(dasm_State** Dst, ir_register_allocation_t dst_reg, ir_register_allocation_t src_reg, ir_value_type_t size);
void host_emit_mov_gpr_fgr(dasm_State** Dst, ir_register_allocation_t dst_reg, ir_register_allocation_t src_reg, ir_value_type_t size);
void host_emit_mov_fgr_fgr(dasm_State** Dst, ir_register_allocation_t dst_reg, ir_register_allocation_t src_reg, ir_float_value_type_t format);
void host_emit_float_convert_reg_reg(dasm_State** Dst, ir_float_value_type_t src_type, ir_register_allocation_t src_reg, ir_float_value_type_t dst_type, ir_register_allocation_t dst_reg);
void host_emit_float_trunc_reg_reg(dasm_State** Dst, ir_float_value_type_t src_type, ir_register_allocation_t src_reg, ir_float_value_type_t dst_type, ir_register_allocation_t dst_reg);
void host_emit_float_round_reg_reg(dasm_State** Dst, ir_float_value_type_t src_type, ir_register_allocation_t src_reg, ir_float_value_type_t dst_type, ir_register_allocation_t dst_reg);
void host_emit_float_add_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc, ir_float_value_type_t format);
void host_emit_float_sub_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc, ir_float_value_type_t format);
void host_emit_float_div_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc, ir_float_value_type_t format);
void host_emit_float_mult_reg_reg(dasm_State** Dst, ir_register_allocation_t operand1_alloc, ir_register_allocation_t operand2_alloc, ir_float_value_type_t format);

void host_emit_float_sqrt_reg_reg(dasm_State** Dst, ir_register_allocation_t dst_alloc, ir_register_allocation_t operand_alloc, ir_float_value_type_t format);
void host_emit_float_abs_reg_reg(dasm_State** Dst, ir_register_allocation_t dst_alloc, ir_register_allocation_t operand_alloc, ir_float_value_type_t format);
void host_emit_float_neg_reg_reg(dasm_State** Dst, ir_register_allocation_t dst_alloc, ir_register_allocation_t operand_alloc, ir_float_value_type_t format);

void host_emit_float_cmp(dasm_State** Dst, ir_float_condition_t condition, ir_float_value_type_t format, ir_register_allocation_t operand1, ir_register_allocation_t operand2);

void host_emit_debugbreak(dasm_State** Dst);
void host_emit_call(dasm_State** Dst, uintptr_t function);

void host_emit_eret(dasm_State** Dst);

void host_emit_interpreter_fallback_until_no_branch(dasm_State** Dst, int extra_cycles);

size_t v2_link(dasm_State** d);
void v2_encode(dasm_State** d, u8* buf);

#endif // N64_V2_EMITTER_H
