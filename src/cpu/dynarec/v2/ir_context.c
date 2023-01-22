#include <log.h>
#include <r4300i.h>
#include "ir_context.h"

ir_context_t ir_context;

void ir_context_reset() {
    for (int i = 0; i < 32; i++) {
        ir_context.guest_gpr_to_value[i] = -1;
    }

    memset(ir_context.ir_cache, 0, sizeof(ir_instruction_t) * IR_CACHE_SIZE);

    ir_context.ir_cache[0].type = IR_SET_CONSTANT;
    ir_context.ir_cache[0].set_constant.type = VALUE_TYPE_64;
    ir_context.ir_cache[0].set_constant.value_64 = 0;

    ir_context.ir_cache_index = 1;
}


void update_guest_reg_mapping(u8 guest_reg, int index) {
    if (guest_reg < 32) {
        ir_context.guest_gpr_to_value[guest_reg] = index;
    }
}

int append_ir_instruction(ir_instruction_t instruction, u8 guest_reg) {
    int index = ir_context.ir_cache_index++;
    ir_context.ir_cache[index] = instruction;
    update_guest_reg_mapping(guest_reg, index);
    return index;
}

int ir_emit_set_constant(ir_set_constant_t value, u8 guest_reg) {
    if (guest_reg == 0) {
        // v0 is always zero, don't emit anything
        return 0;
    }

    bool is_zero = false;
    switch (value.type) {
        case VALUE_TYPE_S16:
            is_zero = value.value_s16 == 0;
            break;
        case VALUE_TYPE_U16:
            is_zero = value.value_u16 == 0;
            break;
        case VALUE_TYPE_S32:
            logfatal("Set constant S32");
            //is_zero = value.value_s32 == 0;
            break;
        case VALUE_TYPE_U32:
            logfatal("Set constant U32");
            //is_zero = value.value_u32 == 0;
            break;
        case VALUE_TYPE_64:
            is_zero = value.value_64 == 0;
            break;
    }
    if (is_zero) {
        update_guest_reg_mapping(guest_reg, 0);
        return 0;
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

int ir_emit_and(int operand, int operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_AND;
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

int ir_emit_load(ir_value_type_t type, int address, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_LOAD;
    instruction.load.type = type;
    instruction.load.address = address;
    return append_ir_instruction(instruction, guest_reg);
}
