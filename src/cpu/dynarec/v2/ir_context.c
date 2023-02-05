#include <log.h>
#include <r4300i.h>
#include "ir_context.h"

ir_context_t ir_context;

void ir_context_reset() {
    for (int i = 0; i < 32; i++) {
        ir_context.guest_gpr_to_value[i] = NULL;
    }

    memset(ir_context.ir_cache, 0, sizeof(ir_instruction_t) * IR_CACHE_SIZE);

    ir_context.ir_cache[0].type = IR_SET_CONSTANT;
    ir_context.ir_cache[0].set_constant.type = VALUE_TYPE_64;
    ir_context.ir_cache[0].set_constant.value_64 = 0;
    ir_context.guest_gpr_to_value[0] = &ir_context.ir_cache[0];

    ir_context.ir_cache_index = 1;

    ir_context.ir_cache_head = &ir_context.ir_cache[0];
    ir_context.ir_cache_tail = &ir_context.ir_cache[0];

    ir_context.block_end_pc_set = false;
}

const char* val_type_to_str(ir_value_type_t type) {
    switch (type) {
        case VALUE_TYPE_U8:
            return "U8";
        case VALUE_TYPE_S8:
            return "S8";
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
        case CONDITION_EQUAL:
            return "==";
        case CONDITION_LESS_THAN:
            return "<";
        case CONDITION_GREATER_THAN:
            return ">";
    }
}

void ir_instr_to_string(ir_instruction_t* instr, char* buf, size_t buf_size) {

    if (instr->type != IR_STORE && instr->type != IR_SET_COND_BLOCK_EXIT_PC && instr->type != IR_SET_BLOCK_EXIT_PC && instr->type != IR_NOP && instr->type != IR_FLUSH_GUEST_REG) {
        int written = snprintf(buf, buf_size, "v%d = ", instr->index);
        buf += written;
        buf_size -= written;
    }

    switch (instr->type) {
        case IR_NOP:
            snprintf(buf, buf_size, "");
            break;
        case IR_SET_CONSTANT:
            switch (instr->set_constant.type) {
                case VALUE_TYPE_U8:
                    snprintf(buf, buf_size, "0x%04X ;%u", (u16)instr->set_constant.value_u8, instr->set_constant.value_u8);
                    break;
                case VALUE_TYPE_S8:
                    snprintf(buf, buf_size, "0x%04X ;%d", (u16)instr->set_constant.value_s8, instr->set_constant.value_s8);
                    break;
                case VALUE_TYPE_S16:
                    snprintf(buf, buf_size, "0x%04X ;%d", (u16)instr->set_constant.value_s16, instr->set_constant.value_s16);
                    break;
                case VALUE_TYPE_U16:
                    snprintf(buf, buf_size, "0x%04X ;%u", instr->set_constant.value_u16, instr->set_constant.value_u16);
                    break;
                case VALUE_TYPE_S32:
                    snprintf(buf, buf_size, "0x%08X ;%d", (u32)instr->set_constant.value_s32, instr->set_constant.value_s32);
                    break;
                case VALUE_TYPE_U32:
                    snprintf(buf, buf_size, "0x%08X ;%u", instr->set_constant.value_u32, instr->set_constant.value_u32);
                    break;
                case VALUE_TYPE_64:
                    snprintf(buf, buf_size, "0x%016lX ;%ld", instr->set_constant.value_64, instr->set_constant.value_64);
                    break;
            }
            break;
        case IR_OR:
            snprintf(buf, buf_size, "v%d | v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_AND:
            snprintf(buf, buf_size, "v%d & v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_ADD:
            snprintf(buf, buf_size, "v%d + v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_STORE:
            snprintf(buf, buf_size, "STORE(type = %s, address = v%d, value = v%d)", val_type_to_str(instr->store.type), instr->store.address->index, instr->store.value->index);
            break;
        case IR_LOAD:
            snprintf(buf, buf_size, "LOAD(type = %s, address = v%d)", val_type_to_str(instr->store.type), instr->store.address->index);
            break;
        case IR_MASK_AND_CAST:
            snprintf(buf, buf_size, "mask_cast(%s, v%d)", val_type_to_str(instr->mask_and_cast.type), instr->mask_and_cast.operand->index);
            break;
        case IR_CHECK_CONDITION:
            snprintf(buf, buf_size, "v%d %s v%d",
                     instr->check_condition.operand1->index,
                     cond_to_str(instr->check_condition.condition),
                     instr->check_condition.operand2->index);
            break;
        case IR_SET_BLOCK_EXIT_PC:
            snprintf(buf, buf_size, "set_block_exit(v%d)", instr->set_exit_pc.address->index);
            break;
        case IR_SET_COND_BLOCK_EXIT_PC:
            snprintf(buf, buf_size, "set_block_exit(v%d, if_true = v%d, if_false = v%d)", instr->set_cond_exit_pc.condition->index, instr->set_cond_exit_pc.pc_if_true->index, instr->set_cond_exit_pc.pc_if_false->index);
            break;
        case IR_TLB_LOOKUP:
            snprintf(buf, buf_size, "tlb_lookup(v%d)", instr->tlb_lookup.virtual_address->index);
            break;
        case IR_LOAD_GUEST_REG:
            snprintf(buf, buf_size, "guest_gpr[%d]", instr->load_guest_reg.guest_reg);
            break;
        case IR_FLUSH_GUEST_REG:
            snprintf(buf, buf_size, "guest_gpr[%d] = v%d", instr->flush_guest_reg.guest_reg, instr->flush_guest_reg.value->index);
            break;
        case IR_SHIFT:
            switch (instr->shift.direction) {
                case SHIFT_DIRECTION_LEFT:
                    snprintf(buf, buf_size, "v%d << v%d", instr->shift.operand->index, instr->shift.amount->index);
                    break;
                case SHIFT_DIRECTION_RIGHT:
                    snprintf(buf, buf_size, "v%d >> v%d", instr->shift.operand->index, instr->shift.amount->index);
                    break;
            }
            break;
    }
}

void update_guest_reg_mapping(u8 guest_reg, ir_instruction_t* value) {
    if (guest_reg < 32) {
        printf("Updating guest reg r%u to value %016lX\n", guest_reg, (uintptr_t)value);
        ir_context.guest_gpr_to_value[guest_reg] = value;
    }
}

ir_instruction_t* append_ir_instruction(ir_instruction_t instruction, u8 guest_reg) {
    int index = ir_context.ir_cache_index++;
    ir_instruction_t* allocation = &ir_context.ir_cache[index];
    *allocation = instruction;

    allocation->next = NULL;
    allocation->prev = ir_context.ir_cache_tail;
    allocation->index = index;
    allocation->dead_code = true; // Will be marked false at the dead code elimination stage
    allocation->allocated_host_register = -1;

    ir_context.ir_cache_tail->next = allocation;
    ir_context.ir_cache_tail = allocation;

    update_guest_reg_mapping(guest_reg, allocation);
    return allocation;
}

ir_instruction_t* ir_emit_set_constant(ir_set_constant_t value, u8 guest_reg) {
    if (guest_reg == 0) {
        // v0 is always zero, don't emit anything, reuse
        return ir_context.ir_cache_head;
    }

    bool is_zero = false;
    switch (value.type) {
        case VALUE_TYPE_S8:
            is_zero = value.value_s8 == 0;
            break;
        case VALUE_TYPE_U8:
            is_zero = value.value_u8 == 0;
            break;
        case VALUE_TYPE_S16:
            is_zero = value.value_s16 == 0;
            break;
        case VALUE_TYPE_U16:
            is_zero = value.value_u16 == 0;
            break;
        case VALUE_TYPE_S32:
            is_zero = value.value_s32 == 0;
            break;
        case VALUE_TYPE_U32:
            is_zero = value.value_u32 == 0;
            break;
        case VALUE_TYPE_64:
            is_zero = value.value_64 == 0;
            break;
    }
    if (is_zero) {
        update_guest_reg_mapping(guest_reg, ir_context.ir_cache_head);
        return ir_context.ir_cache_head;
    }

    ir_instruction_t instruction;
    instruction.type = IR_SET_CONSTANT;
    instruction.set_constant = value;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_load_guest_reg(u8 guest_reg) {
    if (guest_reg > 31) {
        logfatal("ir_emit_load_guest_reg: out of range guest reg value: %d", guest_reg);
    }

    if (ir_context.guest_gpr_to_value[guest_reg] != NULL) {
        return ir_context.guest_gpr_to_value[guest_reg];
    }

    ir_instruction_t instruction;
    instruction.type = IR_LOAD_GUEST_REG;
    instruction.load_guest_reg.guest_reg = guest_reg;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_flush_guest_reg(ir_instruction_t* value, u8 guest_reg) {
    if (guest_reg == 0) {
        logfatal("Should never flush r0");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLUSH_GUEST_REG;
    instruction.flush_guest_reg.guest_reg = guest_reg;
    instruction.flush_guest_reg.value = value;

    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_or(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_OR;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_and(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_AND;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_add(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_ADD;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_shift(ir_instruction_t* operand, ir_instruction_t* amount, ir_value_type_t value_type, ir_shift_direction_t direction, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_SHIFT;
    instruction.shift.operand = operand;
    instruction.shift.amount = amount;
    instruction.shift.type = value_type;
    instruction.shift.direction = direction;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_store(ir_value_type_t type, ir_instruction_t* address, ir_instruction_t* value) {
    ir_instruction_t instruction;
    instruction.type = IR_STORE;
    instruction.store.type = type;
    instruction.store.address = address;
    instruction.store.value = value;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_load(ir_value_type_t type, ir_instruction_t* address, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_LOAD;
    instruction.load.type = type;
    instruction.load.address = address;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_mask_and_cast(ir_instruction_t* operand, ir_value_type_t type, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_MASK_AND_CAST;
    instruction.mask_and_cast.type = type;
    instruction.mask_and_cast.operand = operand;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_check_condition(ir_condition_t condition, ir_instruction_t* operand1, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_CHECK_CONDITION;
    instruction.check_condition.condition = condition;
    instruction.check_condition.operand1 = operand1;
    instruction.check_condition.operand2 = operand2;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_conditional_set_block_exit_pc(ir_instruction_t* condition, ir_instruction_t* pc_if_true, ir_instruction_t* pc_if_false) {
    ir_instruction_t instruction;
    instruction.type = IR_SET_COND_BLOCK_EXIT_PC;
    instruction.set_cond_exit_pc.condition = condition;
    instruction.set_cond_exit_pc.pc_if_true = pc_if_true;
    instruction.set_cond_exit_pc.pc_if_false = pc_if_false;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_set_block_exit_pc(ir_instruction_t* address) {
    ir_instruction_t instruction;
    instruction.type = IR_SET_BLOCK_EXIT_PC;
    instruction.set_exit_pc.address = address;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_interpreter_fallback(int num_instructions) {
    logfatal("Unimplemented: Fall back to interpreter for %d instructions", num_instructions);
}

ir_instruction_t* ir_emit_tlb_lookup(ir_instruction_t* virtual_address, u8 guest_reg, bus_access_t bus_access) {
    ir_instruction_t instruction;
    instruction.type = IR_TLB_LOOKUP;
    instruction.tlb_lookup.virtual_address = virtual_address;
    instruction.tlb_lookup.bus_access = bus_access;
    return append_ir_instruction(instruction, guest_reg);
}
