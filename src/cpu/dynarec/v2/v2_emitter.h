#ifndef N64_V2_EMITTER_H
#define N64_V2_EMITTER_H

#include <dynasm/dasm_proto.h>
#include "ir_context.h"

dasm_State* v2_block_header();
void host_emit_mov_reg_imm(dasm_State** Dst, int reg, ir_set_constant_t imm_value);
void host_emit_mov_reg_reg(dasm_State** Dst, int dst_reg, int src_reg);
void v2_end_block(dasm_State** Dst, int block_length);
void host_emit_call(dasm_State** Dst, uintptr_t function);

#endif // N64_V2_EMITTER_H
