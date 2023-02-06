#include <log.h>
#include <string.h>
#include <mem/n64bus.h>
#include "ir_optimizer.h"
#include "target_platform.h"

bool instr_uses_value(ir_instruction_t* instr, ir_instruction_t* value) {
    switch (instr->type) {
        // Unary ops
        case IR_SET_BLOCK_EXIT_PC:
        case IR_NOT:
            return instr->unary_op.operand == value;

        // Bin ops
        case IR_OR:
        case IR_AND:
        case IR_ADD:
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
        case IR_SET_CP0:
            return instr->set_cp0.value == value;
        case IR_COND_BLOCK_EXIT: {
            if (instr->cond_block_exit.condition == value) {
                return true;
            }
            ir_instruction_flush_t* flush_iter = instr->cond_block_exit.regs_to_flush;
            while (flush_iter != NULL) {
                if (flush_iter->item == value) {
                    return true;
                }
                flush_iter = flush_iter->next;
            }
            return false;
        }

            // No dependencies
        case IR_NOP:
        case IR_SET_CONSTANT:
        case IR_LOAD_GUEST_REG:
        case IR_GET_CP0:
            return false;
    }
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
        case VALUE_TYPE_64:
            return constant.value_64;
    }
}

u64 const_to_u64(ir_instruction_t* constant) {
    return set_const_to_u64(constant->set_constant);
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
    for (int i = 1; i < 32; i++) {
        ir_instruction_t* val = ir_context.guest_gpr_to_value[i];
        if (val) {
            // If the guest reg was just loaded and never modified, don't need to flush it
            if (val->type != IR_LOAD_GUEST_REG) {
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
            case IR_SET_CONSTANT:
            case IR_STORE:
            case IR_LOAD:
            case IR_SET_BLOCK_EXIT_PC:
            case IR_LOAD_GUEST_REG:
            case IR_FLUSH_GUEST_REG:
            case IR_GET_CP0:
            case IR_SET_CP0:
                break;

            case IR_COND_BLOCK_EXIT:
                if (is_constant(instr->cond_block_exit.condition)) {
                    logfatal("Cond block exit with const condition");
                }
                break;

            case IR_NOT:
                if (is_constant(instr->unary_op.operand)) {
                    u64 new_value = ~const_to_u64(instr->unary_op.operand);
                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = new_value;
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
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = operand1 | operand2;
                }
                break;

            case IR_AND:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(instr->bin_op.operand1);
                    u64 operand2 = const_to_u64(instr->bin_op.operand2);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = operand1 & operand2;
                }
                // TODO: check if one operand is constant, if it's zero, and replace with a const zero here
                break;

            case IR_ADD:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(instr->bin_op.operand1);
                    u64 operand2 = const_to_u64(instr->bin_op.operand2);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = operand1 + operand2;
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
                                case VALUE_TYPE_U32:
                                    logfatal("const left 32 bit shift");
                                    break;
                                case VALUE_TYPE_64:
                                    instr->type = IR_SET_CONSTANT;
                                    instr->set_constant.type = VALUE_TYPE_64;
                                    instr->set_constant.value_64 = operand << amount;
                                    break;
                            }
                            break;
                        case SHIFT_DIRECTION_RIGHT:
                            logfatal("const right shift");
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
                        case VALUE_TYPE_64:
                            result = value;
                            break;
                    }
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = result;
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
                    }
                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = result ? 1 : 0;
                }
                break;

            case IR_TLB_LOOKUP:
                if (is_constant(instr->tlb_lookup.virtual_address)) {
                    u64 vaddr = const_to_u64(instr->tlb_lookup.virtual_address);
                    if (!is_tlb(vaddr)) {
                        // If the address is direct mapped, we can translate it at compile time
                        instr->type = IR_SET_CONSTANT;
                        instr->set_constant.type = VALUE_TYPE_U32;
                        instr->set_constant.value_u32 = resolve_virtual_address_or_die(vaddr, -1);
                    }
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
            case IR_SET_CP0:
                instr->dead_code = false;
                instr->set_cp0.value->dead_code = false;
                break;
            case IR_COND_BLOCK_EXIT:
                instr->dead_code = false;
                instr->cond_block_exit.condition->dead_code = false;
                ir_instruction_flush_t* flush_iter = instr->cond_block_exit.regs_to_flush;
                while (flush_iter != NULL) {
                    flush_iter->item->dead_code = false;
                    flush_iter = flush_iter->next;
                }
                break;

            // Unary ops
            case IR_NOT:
                if (!instr->dead_code) {
                    instr->unary_op.operand->dead_code = false;
                }
                break;

            // Bin ops
            case IR_OR:
            case IR_AND:
            case IR_ADD:
                if (!instr->dead_code) {
                    instr->bin_op.operand1->dead_code = false;
                    instr->bin_op.operand2->dead_code = false;
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

            // No dependencies
            case IR_GET_CP0: // Getting a CP0 reg never has side effects
            case IR_NOP:
            case IR_SET_CONSTANT:
            case IR_LOAD_GUEST_REG:
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
        if (instr->type == IR_SET_CONSTANT && instr->set_constant.type == VALUE_TYPE_64) {
            u64 val = instr->set_constant.value_64;
            if (val == (s64)(s16)val) {
                instr->set_constant.type = VALUE_TYPE_S16;
                instr->set_constant.value_s16 = val & 0xFFFF;
            } else if (val == (s64)(s32)val) {
                instr->set_constant.type = VALUE_TYPE_S32;
                instr->set_constant.value_s32 = val & 0xFFFFFFFF;
            }
        }

        instr = instr->next;
    }
}

int first_available_register(bool* available_registers, const int* register_lifetimes, int num_registers) {
    for (int i = 0; i < get_num_preserved_registers(); i++) {
        int reg = get_preserved_registers()[i];
#ifdef N64_LOG_COMPILATIONS
        printf("Trying r%d: available? %d", reg, available_registers[reg]);
        if (!available_registers[reg]) {
            printf(" lifetime? %d", register_lifetimes[reg]);
        }
        printf("\n");
#endif

        if (available_registers[reg] || register_lifetimes[reg] < 0) {
            available_registers[reg] = false;
#ifdef N64_LOG_COMPILATIONS
            printf("Allocated r%d\n", reg);
#endif
            return reg;
        }
    }
    logfatal("No more registers!");
}

bool needs_register_allocated(ir_instruction_t* instr) {
    switch (instr->type) {
        case IR_SET_CONSTANT:
            return !is_valid_immediate(instr->set_constant.type);
        case IR_NOP:
        case IR_SET_COND_BLOCK_EXIT_PC:
        case IR_SET_BLOCK_EXIT_PC:
        case IR_STORE:
        case IR_FLUSH_GUEST_REG:
        case IR_SET_CP0:
        case IR_COND_BLOCK_EXIT:
            return false;

        case IR_TLB_LOOKUP:
        case IR_OR:
        case IR_AND:
        case IR_ADD:
        case IR_LOAD:
        case IR_MASK_AND_CAST:
        case IR_CHECK_CONDITION:
        case IR_LOAD_GUEST_REG:
        case IR_SHIFT:
        case IR_NOT:
        case IR_GET_CP0:
            return true;
    }
}

int value_lifetime(ir_instruction_t* value) {
    int lifetime = 0;
    ir_instruction_t* instr = value->next;
    int times_stepped = 1;

    while (instr) {
        if (instr_uses_value(instr, value)) {
            lifetime = times_stepped;
        }
        instr = instr->next;
        times_stepped++;
    }

    return lifetime;
}

void ir_allocate_registers() {
    int num_registers = get_num_registers();

    bool available_registers[num_registers];
    int register_lifetimes[num_registers];
    for (int i = 0; i < num_registers; i++) {
        available_registers[i] = false;
        register_lifetimes[i] = -1;
    }
    for (int i = 0; i < get_num_preserved_registers(); i++) {
        available_registers[get_preserved_registers()[i]] = true;
    }

    ir_instruction_t* instr = ir_context.ir_cache_head;
    while (instr != NULL) {
        instr->allocated_host_register = -1;
        if (needs_register_allocated(instr)) {
#ifdef N64_LOG_COMPILATIONS
            static char buf[100];
            ir_instr_to_string(instr, buf, 100);
            printf("Allocating register for %s\n", buf);
#endif
            instr->allocated_host_register = first_available_register(available_registers, register_lifetimes, num_registers);
            register_lifetimes[instr->allocated_host_register] = value_lifetime(instr);
#ifdef N64_LOG_COMPILATIONS
            printf("v%d allocated to host register r%d with a lifetime of %d\n", instr->index, instr->allocated_host_register, register_lifetimes[instr->allocated_host_register]);
#endif
        }
        instr = instr->next;
        for (int i = 0; i < num_registers; i++) {
            if (register_lifetimes[i] >= 0) {
                register_lifetimes[i]--;
            }
        }
    }
}
