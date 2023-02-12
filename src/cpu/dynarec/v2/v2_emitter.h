#ifndef N64_V2_EMITTER_H
#define N64_V2_EMITTER_H

#include <dynasm/dasm_proto.h>
#include "ir_context.h"

dasm_State* v2_block_header();

enum args_reversed {
    ARGS_NORMAL_ORDER = 0,
    ARGS_REVERSED = 1 // in this order so `if (args_reversed)` is valid
};

void host_emit_mov_reg_imm(dasm_State** Dst, int reg, ir_set_constant_t imm_value);
void host_emit_mov_reg_reg(dasm_State** Dst, int dst_reg, int src_reg, ir_value_type_t source_value_type);

void host_emit_and_reg_imm(dasm_State** Dst, int operand1, ir_set_constant_t operand2);
void host_emit_and_reg_reg(dasm_State** Dst, int operand1, int operand2);
void host_emit_or_reg_imm(dasm_State** Dst, int operand1, ir_set_constant_t operand2);
void host_emit_or_reg_reg(dasm_State** Dst, int operand1, int operand2);
void host_emit_xor_reg_imm(dasm_State** Dst, int operand1, ir_set_constant_t operand2);
void host_emit_xor_reg_reg(dasm_State** Dst, int operand1, int operand2);
void host_emit_add_reg_imm(dasm_State** Dst, int operand1, ir_set_constant_t operand2);
void host_emit_add_reg_reg(dasm_State** Dst, int operand1, int operand2);
void host_emit_shift_reg_imm(dasm_State** Dst, int reg, ir_value_type_t type, u8 shift_amount, ir_shift_direction_t direction);
void host_emit_shift_reg_reg(dasm_State** Dst, int reg, ir_value_type_t type, int amount_reg, ir_shift_direction_t direction);
void host_emit_not(dasm_State** Dst, int reg);
void host_emit_mult_reg_imm(dasm_State** Dst, int reg, ir_set_constant_t imm, ir_value_type_t multiplicand_type);

void v2_end_block(dasm_State** Dst, int block_length);
void host_emit_cmp_reg_imm(dasm_State** Dst, int dest_reg, ir_condition_t cond, int operand1, ir_set_constant_t operand2, enum args_reversed args_reversed);
void host_emit_cmp_reg_reg(dasm_State** Dst, int dest_reg, ir_condition_t cond, int operand1, int operand2, enum args_reversed args_reversed);
void host_emit_cmov_pc_binary(dasm_State** Dst, int cond_register, ir_instruction_t* if_true, ir_instruction_t* if_false);
void host_emit_mov_pc(dasm_State** Dst, ir_instruction_t* value);
void host_emit_mov_mem_imm(dasm_State** Dst, uintptr_t mem, ir_set_constant_t value, ir_value_type_t write_size);
void host_emit_mov_mem_reg(dasm_State** Dst, uintptr_t mem, int reg, ir_value_type_t type);
void host_emit_mov_reg_mem(dasm_State** Dst, int reg, uintptr_t mem);
void host_emit_mov_reg_cp0(dasm_State** Dst, int reg, int cp0_reg);
void host_emit_mov_cp0_reg(dasm_State** Dst, int cp0_reg, int reg);
void host_emit_cond_ret(dasm_State** Dst, int cond_reg, ir_instruction_flush_t* flush_iter, int block_length);


void host_emit_call(dasm_State** Dst, uintptr_t function);

#endif // N64_V2_EMITTER_H
