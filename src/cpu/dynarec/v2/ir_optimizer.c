#include <log.h>
#include <string.h>
#include <mem/n64bus.h>
#include "ir_optimizer.h"
#include "target_platform.h"

u64 set_const_to_u64(ir_set_constant_t constant) {
    switch (constant.type) {
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
            case IR_SET_COND_BLOCK_EXIT_PC:
            case IR_LOAD_GUEST_REG:
            case IR_FLUSH_GUEST_REG:
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
                if (binop_constant(instr)) {
                    logfatal("const shift");
                }
                break;

            case IR_MASK_AND_CAST:
                if (is_constant(instr->mask_and_cast.operand)) {
                    u64 value = const_to_u64(instr->mask_and_cast.operand);
                    u64 result;
                    instr->type = IR_SET_CONSTANT;
                    switch (instr->mask_and_cast.type) {
                        case VALUE_TYPE_S16:
                            result = (s64)(s16)(value & 0xFFFF);
                            break;
                        case VALUE_TYPE_U16:
                            result = value & 0xFFFF;
                            break;
                        case VALUE_TYPE_S32:
                            logfatal("Unimplemented");
                            break;
                        case VALUE_TYPE_U32:
                            logfatal("Unimplemented");
                            break;
                        case VALUE_TYPE_64:
                            result = value;
                            break;
                    }
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = result;
                }
                break;

            case IR_CHECK_CONDITION: // TODO
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
                instr->set_exit_pc.address->dead_code = false;
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
            printf("Can eliminate v%d\n", instr->index);
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
                printf("%016lX is actually a sign-extended 16 bit value!\n", val);

                instr->set_constant.type = VALUE_TYPE_S16;
                instr->set_constant.value_s16 = val & 0xFFFF;
            } else if (val == (s64)(s32)val) {
                printf("%016lX is actually a sign-extended 32 bit value!\n", val);

                instr->set_constant.type = VALUE_TYPE_S32;
                instr->set_constant.value_s32 = val & 0xFFFFFFFF;
            }
        }

        instr = instr->next;
    }
}

int first_available_register(bool* available_registers, const int* register_lifetimes, int num_registers) {
    for (int i = 0; i < num_registers; i++) {
        if (available_registers[i] || register_lifetimes[i] < 0) {
            available_registers[i] = false;
            return i;
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
            return true;
    }
}

bool instr_uses_value(ir_instruction_t* instr, ir_instruction_t* value) {
    switch (instr->type) {
        case IR_STORE:
            return instr->store.address == value || instr->store.value == value;
        case IR_LOAD:
            return instr->load.address == value;
        case IR_SET_BLOCK_EXIT_PC:
            return instr->set_exit_pc.address == value;
        case IR_SET_COND_BLOCK_EXIT_PC:
            return instr->set_cond_exit_pc.condition == value || instr->set_cond_exit_pc.pc_if_true == value || instr->set_cond_exit_pc.pc_if_false == value;
        case IR_FLUSH_GUEST_REG:
            return instr->flush_guest_reg.value == value;

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

        // No dependencies
        case IR_NOP:
        case IR_SET_CONSTANT:
        case IR_LOAD_GUEST_REG:
            return false;
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
            instr->allocated_host_register = first_available_register(available_registers, register_lifetimes, num_registers);
            register_lifetimes[instr->allocated_host_register] = value_lifetime(instr);
            printf("v%d allocated to host register r%d with a lifetime of %d\n", instr->index, instr->allocated_host_register, register_lifetimes[instr->allocated_host_register]);
        }
        instr = instr->next;
        for (int i = 0; i < num_registers; i++) {
            if (register_lifetimes[i] >= 0) {
                register_lifetimes[i]--;
            }
        }
    }
}
