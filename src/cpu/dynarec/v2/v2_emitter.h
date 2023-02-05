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
void host_emit_mov_reg_reg(dasm_State** Dst, int dst_reg, int src_reg);

void host_emit_and_reg_imm(dasm_State** Dst, int reg, ir_set_constant_t imm_value);

void v2_end_block(dasm_State** Dst, int block_length);
void host_emit_cmp_reg_imm(dasm_State** Dst, int allocated_host_register, ir_condition_t cond, int reg, ir_set_constant_t imm_value, enum args_reversed args_reversed);
void host_emit_cmov_pc_binary(dasm_State** Dst, int cond_register, ir_instruction_t* if_true, ir_instruction_t* if_false);
void host_emit_mov_mem_imm(dasm_State** Dst, uintptr_t mem, ir_set_constant_t value);
void host_emit_mov_mem_reg(dasm_State** Dst, uintptr_t mem, int reg);
void host_emit_mov_reg_mem(dasm_State** Dst, int reg, uintptr_t mem);

void host_emit_call(dasm_State** Dst, uintptr_t function);

#endif // N64_V2_EMITTER_H
