#include <log.h>
#include <string.h>
#include <mem/n64bus.h>
#include "ir_optimizer.h"
#include "target_platform.h"

bool instr_uses_value(ir_instruction_t* instr, ir_instruction_t* value) {
    for (int i = 0; i < instr->flush_info.num_regs; i++) {
        if (instr->flush_info.regs[i].item == value) {
            return true;
        }
    }

    switch (instr->type) {
        // Unary ops
        case IR_SET_BLOCK_EXIT_PC:
        case IR_NOT:
            return instr->unary_op.operand == value;

        // Bin ops
        case IR_OR:
        case IR_AND:
        case IR_ADD:
        case IR_SUB:
        case IR_XOR:
            return instr->bin_op.operand1 == value || instr->bin_op.operand2 == value;

        // Other
        case IR_MASK_AND_CAST:
            return instr->mask_and_cast.operand == value;
        case IR_CHECK_CONDITION:
            return instr->check_condition.operand1 == value || instr->check_condition.operand2 == value;
        case IR_TLB_LOOKUP:
            return instr->tlb_lookup.virtual_address == value;
        case IR_SHIFT:
            return instr->shift.operand == value || instr->shift.amount == value;
        case IR_STORE:
            return instr->store.address == value || instr->store.value == value;
        case IR_LOAD:
            return instr->load.address == value;
        case IR_SET_COND_BLOCK_EXIT_PC:
            return instr->set_cond_exit_pc.condition == value || instr->set_cond_exit_pc.pc_if_true == value || instr->set_cond_exit_pc.pc_if_false == value;
        case IR_FLUSH_GUEST_REG:
            return instr->flush_guest_reg.value == value;
        case IR_COND_BLOCK_EXIT: {
            if (instr->cond_block_exit.condition == value) {
                return true;
            }
            switch (instr->cond_block_exit.type) {
                case COND_BLOCK_EXIT_TYPE_NONE:
                case COND_BLOCK_EXIT_TYPE_EXCEPTION:
                    break;
                case COND_BLOCK_EXIT_TYPE_ADDRESS:
                    if (instr->cond_block_exit.info.exit_pc == value) {
                        return true;
                    }
                    break;
            }
            return false;
        }
        case IR_MULTIPLY:
        case IR_DIVIDE:
            return instr->mult_div.operand1 == value || instr->mult_div.operand2 == value;
        case IR_SET_PTR:
            return instr->set_ptr.value == value;
        case IR_MOV_REG_TYPE:
            return instr->mov_reg_type.value == value;
        case IR_FLOAT_CONVERT:
            return instr->float_convert.value == value;
        case IR_FLOAT_CHECK_CONDITION:
            return instr->float_check_condition.operand1 == value || instr->float_check_condition.operand2 == value;
        case IR_CALL:
            for (int i = 0; i < instr->call.num_args; i++) {
                if (instr->call.arguments[i] == value) {
                    return true;
                }
            }
            return false;

        // Float bin ops
        case IR_FLOAT_DIVIDE:
        case IR_FLOAT_MULTIPLY:
        case IR_FLOAT_ADD:
        case IR_FLOAT_SUB:
            return instr->float_bin_op.operand1 == value || instr->float_bin_op.operand2 == value;

        // Float unary ops
        case IR_FLOAT_SQRT:
        case IR_FLOAT_ABS:
        case IR_FLOAT_NEG:
            return instr->float_unary_op.operand == value;

        // No dependencies
        case IR_ERET:
        case IR_GET_PTR:
        case IR_NOP:
        case IR_SET_CONSTANT:
        case IR_SET_FLOAT_CONSTANT:
        case IR_LOAD_GUEST_REG:
        case IR_INTERPRETER_FALLBACK:
            return false;
    }
    logfatal("Did not match any cases");
}

u32 set_const_to_u32(ir_set_constant_t constant) {
    return (u32)(set_const_to_u64(constant) & 0xFFFFFFFF);
}

s32 set_const_to_s32(ir_set_constant_t constant) {
    return (s32)(set_const_to_u64(constant) & 0xFFFFFFFF);
}

u64 set_const_to_u64(ir_set_constant_t constant) {
    switch (constant.type) {
        case VALUE_TYPE_S8:
            return (s64)constant.value_s8;
        case VALUE_TYPE_U8:
            return constant.value_u8;
        case VALUE_TYPE_S16:
            return (s64)constant.value_s16;
        case VALUE_TYPE_U16:
            return constant.value_u16;
        case VALUE_TYPE_S32:
            return (s64)constant.value_s32;
        case VALUE_TYPE_U32:
            return constant.value_u32;
        case VALUE_TYPE_U64:
            return constant.value_u64;
        case VALUE_TYPE_S64:
            return constant.value_s64;
    }
}

s64 set_const_to_s64(ir_set_constant_t constant) {
    return (s64)set_const_to_u64(constant);
}

u64 const_to_u64(ir_instruction_t* constant) {
    unimplemented(constant->type != IR_SET_CONSTANT, "const_to_u64 on non-IR_SET_CONSTANT instruction");
    return set_const_to_u64(constant->set_constant);
}

u64 set_float_const_to_u64(ir_set_float_constant_t constant) {
    u64 result = 0;
    switch (constant.format) {
        case FLOAT_VALUE_TYPE_INVALID:
            logfatal("set_float_const_to_u64 FLOAT_VALUE_TYPE_INVALID");
            break;
        case FLOAT_VALUE_TYPE_WORD:
            logfatal("set_float_const_to_u64 FLOAT_VALUE_TYPE_WORD");
            break;
        case FLOAT_VALUE_TYPE_LONG:
            result = constant.value_long;
            break;
        case FLOAT_VALUE_TYPE_SINGLE:
            logfatal("set_float_const_to_u64 FLOAT_VALUE_TYPE_SINGLE");
            break;
        case FLOAT_VALUE_TYPE_DOUBLE:
            logfatal("set_float_const_to_u64 FLOAT_VALUE_TYPE_DOUBLE");
            break;
    }
    return result;
}

u64 float_const_to_u64(ir_instruction_t* constant) {
    unimplemented(constant->type != IR_SET_FLOAT_CONSTANT, "float_const_to_u64 on non-IR_SET_FLOAT_CONSTANT instruction");
    return set_float_const_to_u64(constant->set_float_constant);
}

ir_instruction_t* last_value_usage(ir_instruction_t* value) {
    ir_instruction_t* last_usage = value;
    ir_instruction_t* iter = value->next;

    while (iter != NULL) {
        if (instr_uses_value(iter, value)) {
            last_usage = iter;
        }
        iter = iter->next;
    }

    return last_usage;
}

void ir_optimize_flush_guest_regs() {
    // Flush all guest regs in use at the end
    for (int i = 1; i < 64; i++) {
        ir_instruction_t* val = ir_context.guest_reg_to_value[i];
        if (val) {
            // If the guest reg was just loaded and never modified, don't need to flush it
            if (val->type != IR_LOAD_GUEST_REG || val->load_guest_reg.guest_reg != i) {
                ir_instruction_t* last_usage = last_value_usage(val);
                ir_emit_flush_guest_reg(last_usage, val, i);
            }
        }
    }
}

void ir_optimize_constant_propagation() {
    ir_instruction_t* instr = ir_context.ir_cache_head;
    while (instr != NULL) {

        switch (instr->type) {
            // Unable to be optimized further
            case IR_NOP:
            case IR_ERET:
            case IR_SET_CONSTANT:
            case IR_SET_FLOAT_CONSTANT:
            case IR_STORE:
            case IR_LOAD:
            case IR_GET_PTR:
            case IR_SET_PTR:
            case IR_SET_BLOCK_EXIT_PC:
            case IR_LOAD_GUEST_REG:
            case IR_FLUSH_GUEST_REG:
            case IR_INTERPRETER_FALLBACK:
            case IR_COND_BLOCK_EXIT: // Const condition checked in compiler
            // TODO
            case IR_MULTIPLY:
            case IR_DIVIDE:
            case IR_CALL:
                break;

            case IR_MOV_REG_TYPE:
                if (is_constant(instr->mov_reg_type.value)) {
                    u64 const_val = set_const_to_u64(instr->mov_reg_type.value->set_constant);
                    switch (instr->mov_reg_type.new_type) {
                        case REGISTER_TYPE_NONE:
                            logfatal("new_type cannot be REGISTER_TYPE_NONE");
                            break;
                        case REGISTER_TYPE_GPR:
                            logfatal("mov_reg_type from GPR to GPR?");
                            break;
                        case REGISTER_TYPE_FGR_32:
                            instr->type = IR_SET_FLOAT_CONSTANT;
                            instr->set_float_constant.format = FLOAT_VALUE_TYPE_WORD;
                            instr->set_float_constant.value_word = const_val;
                            break;
                        case REGISTER_TYPE_FGR_64:
                            instr->type = IR_SET_FLOAT_CONSTANT;
                            instr->set_float_constant.format = FLOAT_VALUE_TYPE_LONG;
                            instr->set_float_constant.value_long = const_val;
                            break;
                    }
                } else if (float_is_constant(instr->mov_reg_type.value)) {
                    u64 bin_const_val = float_const_to_u64(instr->mov_reg_type.value);
                    switch (instr->mov_reg_type.new_type) {
                        case REGISTER_TYPE_NONE:
                            logfatal("new_type cannot be REGISTER_TYPE_NONE");
                            break;
                        case REGISTER_TYPE_GPR:
                            instr->type = IR_SET_CONSTANT;
                            instr->set_constant.type = VALUE_TYPE_U64;
                            instr->set_constant.value_u64 = bin_const_val;
                            break;
                        case REGISTER_TYPE_FGR_32:
                            logfatal("Constant propagation for mov_reg_type from FGR to FGR_32");
                            break;
                        case REGISTER_TYPE_FGR_64:
                            logfatal("Constant propagation for mov_reg_type from FGR to FGR_32");
                            break;
                    }
                }
                break;

            case IR_NOT:
                if (is_constant(instr->unary_op.operand)) {
                    ir_set_constant_t value = instr->unary_op.operand->set_constant;
                    instr->type = IR_SET_CONSTANT;
                    s64 temp;
                    switch (value.type) {
                        case VALUE_TYPE_U8:
                            temp = value.value_u8;
                            break;
                        case VALUE_TYPE_S8:
                            temp = value.value_s8;
                            break;
                        case VALUE_TYPE_S16:
                            temp = value.value_s16;
                            break;
                        case VALUE_TYPE_U16:
                            temp = value.value_u16;
                            break;
                        case VALUE_TYPE_S32:
                            temp = value.value_s32;
                            break;
                        case VALUE_TYPE_U32:
                            temp = value.value_u32;
                            break;
                        case VALUE_TYPE_U64:
                            temp = value.value_u64;
                            break;
                        case VALUE_TYPE_S64:
                            temp = value.value_s64;
                            break;
                    }
                    value.value_s64 = ~temp;
                    value.type = VALUE_TYPE_S64;
                    instr->set_constant = value;
                }
                break;

            case IR_SET_COND_BLOCK_EXIT_PC:
                if (is_constant(instr->set_cond_exit_pc.condition)) {
                    u64 cond = const_to_u64(instr->set_cond_exit_pc.condition);
                    ir_instruction_t* if_false = instr->set_cond_exit_pc.pc_if_false;
                    ir_instruction_t* if_true  = instr->set_cond_exit_pc.pc_if_true;
                    instr->type = IR_SET_BLOCK_EXIT_PC;
                    instr->unary_op.operand = cond != 0 ? if_true : if_false;
                }
                break;

            case IR_OR:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(instr->bin_op.operand1);
                    u64 operand2 = const_to_u64(instr->bin_op.operand2);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_U64;
                    instr->set_constant.value_u64 = operand1 | operand2;
                }
                break;

            case IR_XOR:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(instr->bin_op.operand1);
                    u64 operand2 = const_to_u64(instr->bin_op.operand2);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_U64;
                    instr->set_constant.value_u64 = operand1 ^ operand2;
                }
                break;

            case IR_AND:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(instr->bin_op.operand1);
                    u64 operand2 = const_to_u64(instr->bin_op.operand2);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_U64;
                    instr->set_constant.value_u64 = operand1 & operand2;
                }
                // TODO: check if one operand is constant, if it's zero, and replace with a const zero here
                break;

            case IR_ADD:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(instr->bin_op.operand1);
                    u64 operand2 = const_to_u64(instr->bin_op.operand2);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_U64;
                    instr->set_constant.value_u64 = operand1 + operand2;
                }
                break;

            case IR_SUB:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(instr->bin_op.operand1);
                    u64 operand2 = const_to_u64(instr->bin_op.operand2);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_U64;
                    instr->set_constant.value_u64 = operand1 - operand2;
                }
                break;

            case IR_SHIFT:
                if (is_constant(instr->shift.operand) && is_constant(instr->shift.amount)) {
                    u64 operand = const_to_u64(instr->shift.operand);
                    u64 amount_64 = const_to_u64(instr->shift.amount);
                    if (amount_64 > 63) {
                        logfatal("const shift amount (%lu) much too large - something is wrong", amount_64);
                    }
                    u8 amount = amount_64 & 0xFF;

                    switch (instr->shift.direction) {
                        case SHIFT_DIRECTION_LEFT:
                            switch (instr->shift.type) {
                                case VALUE_TYPE_U8:
                                case VALUE_TYPE_S8:
                                    logfatal("const left 8 bit shift");
                                    break;
                                case VALUE_TYPE_S16:
                                case VALUE_TYPE_U16:
                                    logfatal("const left 16 bit shift");
                                    break;
                                case VALUE_TYPE_S32:
                                    instr->type = IR_SET_CONSTANT;
                                    instr->set_constant.type = VALUE_TYPE_S32;
                                    instr->set_constant.value_s32 = (s32)operand << amount;
                                    break;
                                case VALUE_TYPE_U32:
                                    instr->type = IR_SET_CONSTANT;
                                    instr->set_constant.type = VALUE_TYPE_U32;
                                    instr->set_constant.value_u32 = (u32)operand << amount;
                                    break;
                                case VALUE_TYPE_U64:
                                    instr->type = IR_SET_CONSTANT;
                                    instr->set_constant.type = VALUE_TYPE_U64;
                                    instr->set_constant.value_u64 = operand << amount;
                                    break;
                                case VALUE_TYPE_S64:
                                    logfatal("const left 64 bit signed shift");
                                    break;
                            }
                            break;
                        case SHIFT_DIRECTION_RIGHT:
                            switch (instr->shift.type) {
                                case VALUE_TYPE_U8:
                                case VALUE_TYPE_S8:
                                    logfatal("const right 8 bit shift");
                                    break;
                                case VALUE_TYPE_S16:
                                case VALUE_TYPE_U16:
                                    logfatal("const right 16 bit shift");
                                    break;
                                case VALUE_TYPE_S32:
                                    logfatal("const right s32 shift");
                                    break;
                                case VALUE_TYPE_U32:
                                    instr->type = IR_SET_CONSTANT;
                                    instr->set_constant.type = VALUE_TYPE_U32;
                                    instr->set_constant.value_u32 = (u32)operand >> amount;
                                    break;
                                case VALUE_TYPE_U64:
                                    instr->type = IR_SET_CONSTANT;
                                    instr->set_constant.type = VALUE_TYPE_U64;
                                    instr->set_constant.value_u64 = (u64)operand >> amount;
                                    break;
                                case VALUE_TYPE_S64:
                                    instr->type = IR_SET_CONSTANT;
                                    instr->set_constant.type = VALUE_TYPE_S64;
                                    instr->set_constant.value_s64 = (s64)operand >> amount;
                                    break;
                            }
                            break;
                    }
                }
                break;

            case IR_MASK_AND_CAST:
                if (is_constant(instr->mask_and_cast.operand)) {
                    u64 value = const_to_u64(instr->mask_and_cast.operand);
                    u64 result;
                    instr->type = IR_SET_CONSTANT;
                    switch (instr->mask_and_cast.type) {
                        case VALUE_TYPE_S8:
                            result = (s64)(s8)(value & 0xFF);
                            break;
                        case VALUE_TYPE_U8:
                            result = value & 0xFF;
                            break;
                        case VALUE_TYPE_S16:
                            result = (s64)(s16)(value & 0xFFFF);
                            break;
                        case VALUE_TYPE_U16:
                            result = value & 0xFFFF;
                            break;
                        case VALUE_TYPE_S32:
                            result = (s64)(s32)(value & 0xFFFFFFFF);
                            break;
                        case VALUE_TYPE_U32:
                            result = value & 0xFFFFFFFF;
                            break;
                        case VALUE_TYPE_U64:
                        case VALUE_TYPE_S64:
                            result = value;
                            break;
                    }
                    instr->set_constant.type = VALUE_TYPE_U64;
                    instr->set_constant.value_u64 = result;
                }
                break;

            case IR_CHECK_CONDITION:
                if (is_constant(instr->check_condition.operand1) && is_constant(instr->check_condition.operand2)) {
                    u64 operand1 = const_to_u64(instr->check_condition.operand1);
                    u64 operand2 = const_to_u64(instr->check_condition.operand2);
                    bool result = false;
                    switch (instr->check_condition.condition) {
                        case CONDITION_NOT_EQUAL:
                            result = operand1 != operand2;
                            break;
                        case CONDITION_EQUAL:
                            result = operand1 == operand2;
                            break;
                        case CONDITION_LESS_THAN_SIGNED:
                            result = (s64)operand1 < (s64)operand2;
                            break;
                        case CONDITION_LESS_THAN_UNSIGNED:
                            result = operand1 < operand2;
                            break;
                        case CONDITION_GREATER_THAN_SIGNED:
                            result = (s64)operand1 > (s64)operand2;
                            break;
                        case CONDITION_GREATER_THAN_UNSIGNED:
                            result = operand1 > operand2;
                            break;
                        case CONDITION_LESS_OR_EQUAL_TO_SIGNED:
                            result = (s64)operand1 <= (s64)operand2;
                            break;
                        case CONDITION_LESS_OR_EQUAL_TO_UNSIGNED:
                            result = operand1 <= operand2;
                            break;
                        case CONDITION_GREATER_OR_EQUAL_TO_SIGNED:
                            result = (s64)operand1 >= (s64)operand2;
                            break;
                        case CONDITION_GREATER_OR_EQUAL_TO_UNSIGNED:
                            result = operand1 >= operand2;
                            break;
                    }
                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_U64;
                    instr->set_constant.value_u64 = result ? 1 : 0;
                }
                break;

            case IR_TLB_LOOKUP:
                if (is_constant(instr->tlb_lookup.virtual_address)) {
                    u64 vaddr = const_to_u64(instr->tlb_lookup.virtual_address);
                    if (!is_tlb(vaddr)) {
                        // If the address is direct mapped, we can translate it at compile time
                        bus_access_t bus_access = instr->tlb_lookup.bus_access;
                        instr->type = IR_SET_CONSTANT;
                        instr->set_constant.type = VALUE_TYPE_U32;
                        instr->set_constant.value_u32 = resolve_virtual_address_or_die(vaddr, bus_access);
                    }
                }
                break;
            case IR_FLOAT_CONVERT:
                if (float_is_constant(instr->float_convert.value)) {
                    logwarn("Constant propagation for IR_FLOAT_CONVERT");
                }
                break;
            case IR_FLOAT_MULTIPLY:
                if (float_binop_constant(instr)) {
                    logfatal("Constant propagation for IR_FLOAT_MULTIPLY");
                }
                break;
            case IR_FLOAT_DIVIDE:
                if (float_binop_constant(instr)) {
                    logfatal("Constant propagation for IR_FLOAT_DIVIDE");
                }
                break;
            case IR_FLOAT_ADD:
                if (float_binop_constant(instr)) {
                    logfatal("Constant propagation for IR_FLOAT_ADD");
                }
                break;
            case IR_FLOAT_SUB:
                if (float_binop_constant(instr)) {
                    logfatal("Constant propagation for IR_FLOAT_SUB");
                }
                break;
            case IR_FLOAT_CHECK_CONDITION:
                if (float_is_constant(instr->float_check_condition.operand1) && float_is_constant(instr->float_check_condition.operand2)) {
                    logfatal("Constant propagation for IR_FLOAT_CHECK_CONDITION");
                }
                break;

            case IR_FLOAT_ABS:
                if (float_is_constant(instr->float_unary_op.operand)) {
                    logfatal("Constant propagation for IR_FLOAT_ABS");
                }
                break;
            case IR_FLOAT_SQRT:
                if (float_is_constant(instr->float_unary_op.operand)) {
                    logfatal("Constant propagation for IR_FLOAT_SQRT");
                }
                break;
            case IR_FLOAT_NEG:
                if (float_is_constant(instr->float_unary_op.operand)) {
                    logfatal("Constant propagation for IR_FLOAT_NEG");
                }
                break;
        }

        instr = instr->next;
    }
}

void ir_optimize_eliminate_dead_code() {
    ir_instruction_t* instr = ir_context.ir_cache_tail;
    // Loop through instructions backwards
    while (instr != NULL) {
        // Reg values preserved for flushing upon early exit
        for (int i = 0; i < instr->flush_info.num_regs; i++) {
            instr->flush_info.regs[i].item->dead_code = false;
        }

        switch (instr->type) {
            // Never eliminated
            case IR_STORE:
                instr->store.address->dead_code = false;
                instr->store.value->dead_code = false;
                instr->dead_code = false;
                break;
            case IR_LOAD:
                instr->load.address->dead_code = false;
                instr->dead_code = false;
                break;
            case IR_SET_BLOCK_EXIT_PC:
                instr->unary_op.operand->dead_code = false;
                instr->dead_code = false;
                break;
            case IR_SET_COND_BLOCK_EXIT_PC:
                instr->set_cond_exit_pc.condition->dead_code = false;
                instr->set_cond_exit_pc.pc_if_true->dead_code = false;
                instr->set_cond_exit_pc.pc_if_false->dead_code = false;
                instr->dead_code = false;
                break;
            case IR_FLUSH_GUEST_REG:
                instr->flush_guest_reg.value->dead_code = false;
                instr->dead_code = false;
                break;
            case IR_COND_BLOCK_EXIT:
                instr->dead_code = false;
                instr->cond_block_exit.condition->dead_code = false;
                break;
            case IR_MULTIPLY:
            case IR_DIVIDE:
                instr->mult_div.operand1->dead_code = false;
                instr->mult_div.operand2->dead_code = false;
                instr->dead_code = false;
                break;
            case IR_SET_PTR:
                instr->dead_code = false;
                instr->set_ptr.value->dead_code = false;
                break;
            case IR_ERET:
                instr->dead_code = false;
                break;
            case IR_FLOAT_CHECK_CONDITION:
                instr->dead_code = false;
                instr->float_check_condition.operand1->dead_code = false;
                instr->float_check_condition.operand2->dead_code = false;
                break;
            case IR_CALL:
                instr->dead_code = false;
                for (int i = 0; i < instr->call.num_args; i++) {
                    instr->call.arguments[i]->dead_code = false;
                }
                break;
            case IR_INTERPRETER_FALLBACK:
                instr->dead_code = false;
                break;

            // Unary ops
            case IR_NOT:
                if (!instr->dead_code) {
                    instr->unary_op.operand->dead_code = false;
                }
                break;

            // Bin ops
            case IR_OR:
            case IR_XOR:
            case IR_AND:
            case IR_ADD:
            case IR_SUB:
                if (!instr->dead_code) {
                    instr->bin_op.operand1->dead_code = false;
                    instr->bin_op.operand2->dead_code = false;
                }
                break;

            // Float bin ops
            case IR_FLOAT_DIVIDE:
            case IR_FLOAT_MULTIPLY:
            case IR_FLOAT_ADD:
            case IR_FLOAT_SUB:
                if (!instr->dead_code) {
                    instr->float_bin_op.operand1->dead_code = false;
                    instr->float_bin_op.operand2->dead_code = false;
                }
                break;

            // Float unary ops
            case IR_FLOAT_SQRT:
            case IR_FLOAT_ABS:
            case IR_FLOAT_NEG:
                if (!instr->dead_code) {
                    instr->float_unary_op.operand->dead_code = false;
                }
                break;

            // Other
            case IR_MASK_AND_CAST:
                if (!instr->dead_code) {
                    instr->mask_and_cast.operand->dead_code = false;
                }
                break;
            case IR_CHECK_CONDITION:
                if (!instr->dead_code) {
                    instr->check_condition.operand1->dead_code = false;
                    instr->check_condition.operand2->dead_code = false;
                }
                break;
            case IR_TLB_LOOKUP:
                if (!instr->dead_code) {
                    instr->tlb_lookup.virtual_address->dead_code = false;
                }
                break;
            case IR_SHIFT:
                if (!instr->dead_code) {
                    instr->shift.operand->dead_code = false;
                    instr->shift.amount->dead_code = false;
                }
                break;
            case IR_MOV_REG_TYPE:
                if (!instr->dead_code) {
                    instr->mov_reg_type.value->dead_code = false;
                }
                break;
            case IR_FLOAT_CONVERT:
                if (!instr->dead_code) {
                    instr->float_convert.value->dead_code = false;
                }
                break;

            // No dependencies
            case IR_NOP:
            case IR_SET_CONSTANT:
            case IR_SET_FLOAT_CONSTANT:
            case IR_LOAD_GUEST_REG:
            case IR_GET_PTR:
                break;
        }

        instr = instr->prev;
    }

    instr = ir_context.ir_cache_head;

    while (instr != NULL) {
        if (instr->dead_code) {
            ir_instruction_t* prev = instr->prev;
            ir_instruction_t* next = instr->next;

            if (!prev) {
                logfatal("Eliminating head");
            }

            if (!next) {
                logfatal("Eliminating tail");
            }

            next->prev = prev;
            prev->next = next;
        }
        instr = instr->next;
    }
}

void ir_optimize_shrink_constants() {
    ir_instruction_t* instr = ir_context.ir_cache_head;
    while (instr != NULL) {
        if (instr->type == IR_SET_CONSTANT) {
            u64 val = const_to_u64(instr);
            if (val == (u64)(u16)val) { // U16
                instr->set_constant.type = VALUE_TYPE_U16;
                instr->set_constant.value_u16 = val & 0xFFFF;
            } else if (val == (s64)(s16)val) { // S16
                instr->set_constant.type = VALUE_TYPE_S16;
                instr->set_constant.value_s16 = val & 0xFFFF;
            } else if (val == (s64)(u32)val) { // U32
                instr->set_constant.type = VALUE_TYPE_U32;
                instr->set_constant.value_u32 = val & 0xFFFFFFFF;
            } else if (val == (s64)(s32)val) { // S32
                instr->set_constant.type = VALUE_TYPE_S32;
                instr->set_constant.value_s32 = val & 0xFFFFFFFF;
            }
        }

        instr = instr->next;
    }
}

