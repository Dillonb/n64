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
    ir_context.guest_gpr_to_value[0] = 0;

    ir_context.ir_cache_index = 1;
}

const char* val_type_to_str(ir_value_type_t type) {
    switch (type) {
        case VALUE_TYPE_S16:
            return "S16";
        case VALUE_TYPE_U16:
            return "U16";
        case VALUE_TYPE_S32:
            return "S32";
        case VALUE_TYPE_U32:
            return "U32";
        case VALUE_TYPE_64:
            return "_64";
    }
}

const char* cond_to_str(ir_condition_t condition) {
    switch (condition) {
        case CONDITION_NOT_EQUAL:
            return "!=";
    }
}

void ir_instr_to_string(int index, char* buf, size_t buf_size) {
    ir_instruction_t instr = ir_context.ir_cache[index];

    if (instr.type != IR_STORE && instr.type != IR_SET_BLOCK_EXIT_PC && instr.type != IR_NOP) {
        int written = snprintf(buf, buf_size, "v%d = ", index);
        buf += written;
        buf_size -= written;
    }

    switch (instr.type) {
        case IR_NOP:
            snprintf(buf, buf_size, "");
            break;
        case IR_SET_CONSTANT:
            switch (instr.set_constant.type) {
                case VALUE_TYPE_S16:
                    snprintf(buf, buf_size, "0x%04X ;%d", (u16)instr.set_constant.value_s16, instr.set_constant.value_s16);
                    break;
                case VALUE_TYPE_U16:
                    snprintf(buf, buf_size, "0x%04X ;%u", instr.set_constant.value_u16, instr.set_constant.value_u16);
                    break;
                case VALUE_TYPE_S32:
                    logfatal("set const s32 to string");
                    //snprintf(buf, buf_size, "0x%08X ;%d", (u32)instr.set_constant.value_s32, instr.set_constant.value_s32);
                    break;
                case VALUE_TYPE_U32:
                    logfatal("set const u32 to string");
                    //snprintf(buf, buf_size, "0x%08X ;%u", instr.set_constant.value_u32, instr.set_constant.value_u32);
                    break;
                case VALUE_TYPE_64:
                    snprintf(buf, buf_size, "0x%016lX ;%ld", instr.set_constant.value_64, instr.set_constant.value_64);
                    break;
            }
            break;
        case IR_OR:
            snprintf(buf, buf_size, "v%d | v%d", instr.bin_op.operand1, instr.bin_op.operand2);
            break;
        case IR_AND:
            snprintf(buf, buf_size, "v%d & v%d", instr.bin_op.operand1, instr.bin_op.operand2);
            break;
        case IR_ADD:
            snprintf(buf, buf_size, "v%d + v%d", instr.bin_op.operand1, instr.bin_op.operand2);
            break;
        case IR_STORE:
            snprintf(buf, buf_size, "STORE(type = %s, address = v%d, value = v%d)", val_type_to_str(instr.store.type), instr.store.address, instr.store.value);
            break;
        case IR_LOAD:
            snprintf(buf, buf_size, "LOAD(type = %s, address = v%d)", val_type_to_str(instr.store.type), instr.store.address);
            break;
        case IR_MASK_AND_CAST:
            snprintf(buf, buf_size, "mask_cast(%s, v%d)", val_type_to_str(instr.mask_and_cast.type), instr.mask_and_cast.operand);
            break;
        case IR_CHECK_CONDITION:
            snprintf(buf, buf_size, "v%d %s v%d",
                     instr.check_condition.operand1,
                     cond_to_str(instr.check_condition.condition),
                     instr.check_condition.operand2);
            break;
        case IR_SET_BLOCK_EXIT_PC:
            snprintf(buf, buf_size, "set_block_exit(v%d, if_true = v%d, if_false = v%d)", instr.set_exit_pc.condition, instr.set_exit_pc.pc_if_true, instr.set_exit_pc.pc_if_false);
            break;
    }
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

int ir_emit_mask_and_cast(int operand, ir_value_type_t type, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_MASK_AND_CAST;
    instruction.mask_and_cast.type = type;
    instruction.mask_and_cast.operand = operand;
    return append_ir_instruction(instruction, guest_reg);
}

int ir_emit_check_condition(ir_condition_t condition, int operand1, int operand2) {
    ir_instruction_t instruction;
    instruction.type = IR_CHECK_CONDITION;
    instruction.check_condition.condition = condition;
    instruction.check_condition.operand1 = operand1;
    instruction.check_condition.operand2 = operand2;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

int ir_emit_set_block_exit_pc(int condition, int pc_if_true, int pc_if_false) {
    ir_instruction_t instruction;
    instruction.type = IR_SET_BLOCK_EXIT_PC;
    instruction.set_exit_pc.condition = condition;
    instruction.set_exit_pc.pc_if_true = pc_if_true;
    instruction.set_exit_pc.pc_if_false = pc_if_false;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

int ir_emit_interpreter_fallback(int num_instructions) {
    logfatal("Unimplemented: Fall back to interpreter for %d instructions", num_instructions);
}
