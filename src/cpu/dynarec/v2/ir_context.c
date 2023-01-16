#include <log.h>
#include <r4300i.h>
#include "ir_context.h"

ir_context_t ir_context;

void ir_context_reset() {
    ir_context.ir_cache_index = 0;
    for (int i = 0; i < 32; i++) {
        ir_context.guest_gpr_to_value[i] = -1;
    }

    memset(ir_context.ir_cache, 0, sizeof(ir_instruction_t) * IR_CACHE_SIZE);
}

int append_ir_instruction(ir_instruction_t instruction) {
    int index = ir_context.ir_cache_index++;
    ir_context.ir_cache[index] = instruction;
    return index;
}

void ir_emit_set_register_constant(u8 guest_reg, u64 value) {
    ir_instruction_t instruction;
    instruction.type = IR_SET_REG_CONSTANT;
    instruction.set_reg_constant.value = value;

    int ssa_index = append_ir_instruction(instruction);
    ir_context.guest_gpr_to_value[guest_reg] = ssa_index;
}
