#ifndef N64_IR_OPTIMIZER_H
#define N64_IR_OPTIMIZER_H

#include <util.h>
#include "ir_context.h"

INLINE bool is_constant(ir_instruction_t* instr) {
    return instr->type == IR_SET_CONSTANT;
}

INLINE bool binop_constant(ir_instruction_t* instr) {
    return is_constant(instr->bin_op.operand1) && is_constant(instr->bin_op.operand2);
}


u64 set_const_to_u64(ir_set_constant_t constant);
u64 const_to_u64(ir_instruction_t* constant);

void ir_optimize_constant_propagation();
void ir_optimize_eliminate_dead_code();
void ir_optimize_shrink_constants();
void ir_allocate_registers();

#endif //N64_IR_OPTIMIZER_H
