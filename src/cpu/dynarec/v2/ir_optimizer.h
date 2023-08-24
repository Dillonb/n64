#ifndef N64_IR_OPTIMIZER_H
#define N64_IR_OPTIMIZER_H

#include <util.h>
#include "ir_context.h"
#include <cpu/dynarec/v2/target_platform.h>

#define SPILL_ENTRY_SIZE (sizeof(u64))
#define SPILL_SPACE_NUM_ENTRIES (64)
#define SPILL_SPACE_SIZE_BYTES (SPILL_SPACE_NUM_ENTRIES * SPILL_ENTRY_SIZE)
static_assert((SPILL_SPACE_SIZE_BYTES % 16) == 0, "spill space should not misalign the stack");

INLINE bool is_constant(ir_instruction_t* instr) {
    return instr->type == IR_SET_CONSTANT;
}

INLINE bool float_is_constant(ir_instruction_t* instr) {
    return instr->type == IR_SET_FLOAT_CONSTANT;
}

INLINE bool binop_constant(ir_instruction_t* instr) {
    return is_constant(instr->bin_op.operand1) && is_constant(instr->bin_op.operand2);
}

INLINE bool float_binop_constant(ir_instruction_t* instr) {
    return float_is_constant(instr->float_bin_op.operand1) && float_is_constant(instr->float_bin_op.operand2);
}

// Is the instruction a constant that is also a valid immediate?
INLINE bool instr_valid_immediate(ir_instruction_t* instr) {
    return is_constant(instr) && is_valid_immediate(instr->set_constant.type);
}

// Are both the instruction's arguments constants that are also valid immediates?
INLINE bool binop_valid_immediate(ir_instruction_t* instr) {
    return instr_valid_immediate(instr->bin_op.operand1) && instr_valid_immediate(instr->bin_op.operand2);
}

bool instr_uses_value(ir_instruction_t* instr, ir_instruction_t* value);

u32 set_const_to_u32(ir_set_constant_t constant);
s32 set_const_to_s32(ir_set_constant_t constant);
u64 set_const_to_u64(ir_set_constant_t constant);
s64 set_const_to_s64(ir_set_constant_t constant);

u64 const_to_u64(ir_instruction_t* constant);

u64 set_float_const_to_u64(ir_set_float_constant_t constant);
u64 float_const_to_u64(ir_instruction_t* constant);

void ir_optimize_flush_guest_regs();
void ir_optimize_constant_propagation();
void ir_optimize_eliminate_dead_code();
void ir_optimize_shrink_constants();

#endif //N64_IR_OPTIMIZER_H
