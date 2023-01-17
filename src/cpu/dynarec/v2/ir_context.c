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

int append_ir_instruction(ir_instruction_t instruction, u8 guest_reg) {
    int index = ir_context.ir_cache_index++;
    ir_context.ir_cache[index] = instruction;
    if (guest_reg < 32) {
        ir_context.guest_gpr_to_value[guest_reg] = index;
    }
    return index;
}

int ir_emit_set_constant(ir_set_constant_t value, u8 guest_reg) {
    if (guest_reg == 0) {
        return -1;
    }

    ir_instruction_t instruction;
    instruction.type = IR_SET_CONSTANT;
    instruction.set_constant = value;

    return append_ir_instruction(instruction, guest_reg);
}

int ir_emit_load_guest_reg(u8 guest_reg) {
    if (guest_reg > 31) {
        logfatal("ir_emit_load_guest_reg: out of range guest reg value: %d", guest_reg);
    }

    if (ir_context.guest_gpr_to_value[guest_reg] >= 0) {
        return ir_context.guest_gpr_to_value[guest_reg];
    }

    logfatal("implement me");
}

int ir_emit_or(int operand, int operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_OR;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

int ir_emit_add(int operand, int operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_ADD;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

int ir_emit_store(ir_value_type_t type, int address, int value) {
    ir_instruction_t instruction;
    instruction.type = IR_STORE;
    instruction.store.type = type;
    instruction.store.address = address;
    instruction.store.value = value;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}
