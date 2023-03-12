#include "register_allocator.h"
#include "ir_context.h"
#include "ir_optimizer.h"

typedef struct register_allocation_state {
    int num_active; // number of intervals currently active
    int num_regs; // number of registers available for allocation
    int num_total_regs; // number of total registers on the platform
    bool reg_available[32]; // whether or not a given register is available
    ir_instruction_t* active[32]; // list of active intervals
} register_allocation_state_t;


// How many more instructions this value needs to hang around for
int value_lifetime(ir_instruction_t* value, ir_instruction_t** last_use) {
    int lifetime = 0;
    ir_instruction_t* instr = value->next;
    int times_stepped = 1;

    while (instr) {
        if (instr_uses_value(instr, value)) {
            lifetime = times_stepped;

            if (last_use) {
                *last_use = instr;
            }
        }
        instr = instr->next;
        times_stepped++;
    }

    return lifetime;
}

ir_register_type_t get_required_register_type(ir_instruction_t* instr) {
    switch (instr->type) {
        case IR_SET_CONSTANT:
            return !is_valid_immediate(instr->set_constant.type);
        case IR_NOP:
        case IR_SET_COND_BLOCK_EXIT_PC:
        case IR_SET_BLOCK_EXIT_PC:
        case IR_STORE:
        case IR_FLUSH_GUEST_REG:
        case IR_COND_BLOCK_EXIT:
        case IR_MULTIPLY:
        case IR_DIVIDE:
        case IR_SET_PTR:
        case IR_ERET:
        case IR_FLOAT_CHECK_CONDITION: // uses FCR31.compare.
        case IR_CALL:
            return REGISTER_TYPE_NONE;

        case IR_TLB_LOOKUP:
        case IR_OR:
        case IR_XOR:
        case IR_AND:
        case IR_ADD:
        case IR_SUB:
        case IR_GET_PTR:
        case IR_MASK_AND_CAST:
        case IR_CHECK_CONDITION:
        case IR_SHIFT:
        case IR_NOT:
            return REGISTER_TYPE_GPR;

        case IR_LOAD_GUEST_REG:
            if (IR_IS_FGR(instr->load_guest_reg.guest_reg)) {
                return instr->load_guest_reg.guest_reg_type;
            } else if (IR_IS_GPR(instr->load_guest_reg.guest_reg)) {
                return REGISTER_TYPE_GPR;
            } else {
                logfatal("Unknown register type for guest register %d", instr->load_guest_reg.guest_reg);
            }
            break;

        case IR_LOAD:
            return instr->load.reg_type;

        case IR_MOV_REG_TYPE:
            return instr->mov_reg_type.new_type;

        case IR_FLOAT_CONVERT:
            return float_val_to_reg_type(instr->float_convert.to_type);

        case IR_SET_FLOAT_CONSTANT:
            return float_val_to_reg_type(instr->set_float_constant.format);

        // Float bin ops
        case IR_FLOAT_DIVIDE:
        case IR_FLOAT_MULTIPLY:
        case IR_FLOAT_ADD:
        case IR_FLOAT_SUB:
            return float_val_to_reg_type(instr->float_bin_op.format);
    }
    logfatal("Did not match any cases.");
}

int first_available_register(register_allocation_state_t* state) {
    for (int i = 0; i < state->num_total_regs; i++) {
        if (state->reg_available[i]) {
            return i;
        }
    }
    return -1;
}

int active_value_comparator(const void* a, const void* b) {
    ir_instruction_t* val_a = *(ir_instruction_t**)a;
    ir_instruction_t* val_b = *(ir_instruction_t**)b;
    if (val_a == NULL && val_b == NULL) {
        return 0;
    } else if (val_a == NULL) {
        return 1; // null a comes after b (a - b, 1 - 0, 1)
    } else if (val_b == NULL) {
        return -1; // null b comes after a (a - b, 0 - 1, -1)
    } else {
        return val_a->last_use - val_b->last_use;
    }
}

void ir_recalculate_indices() {
    ir_instruction_t* value = ir_context.ir_cache_head;
    int index = 0;
    while (value != NULL) {
        value->index = index++;
        value = value->next;
    }
}

INLINE void sort_active(register_allocation_state_t* state) {
    if (state->num_active > 0) {
        qsort(state->active, state->num_active, sizeof(ir_instruction_t*), active_value_comparator);
    }
}

void expire_old_intervals(register_allocation_state_t* state, ir_instruction_t* current_value) {
    int num_expired = 0;
    for (int i = 0; i < state->num_active; i++) {
        unimplemented(state->active[i]->reg_alloc.spilled, "Active value marked spilled");
        if (state->active[i]->last_use >= current_value->index) {
            break;
        }
        state->reg_available[state->active[i]->reg_alloc.host_reg] = true;
        state->active[i] = NULL;
        num_expired++;
    }

    if (num_expired > 0) {
        sort_active(state);
        state->num_active -= num_expired;
    }
}

void allocate_register(
        ir_instruction_t* value,
        ir_register_type_t type,
        int* spill_index,
        register_allocation_state_t* state) {
    ir_instruction_t* last_use = NULL;
    value_lifetime(value, &last_use);
    // Value never used, but dead code elimination didn't get rid of it - might be a LOAD, or something else that has a side effect
    if (!last_use) {
        last_use = value;
    }
    value->last_use = last_use->index;

    if (state->num_active == state->num_regs) {
        int active_index = state->num_active - 1; // last entry in active
        ir_instruction_t* spill = state->active[active_index];
        // If the spilled value is used for longer than the value we're trying to allocate, spill the existing value instead of the new value
        if (spill->last_use > value->last_use) {
            // Transfer register allocation information from the spilled register to the new reg
            value->reg_alloc = spill->reg_alloc;

            // Allocate a new space on the stack for the spilled value
            spill->reg_alloc.spilled = true;
            spill->reg_alloc.spill_location = *spill_index;
            *spill_index += SPILL_ENTRY_SIZE;

            // Replace spilled value in active list with the new value
            state->active[active_index] = value;
        } else {
            // Allocate a new space on the stack for the new value
            value->reg_alloc = alloc_reg_spilled(*spill_index, type);
            *spill_index += SPILL_ENTRY_SIZE;
        }
    } else {
        int reg = first_available_register(state);
        if (reg < 0) {
            logfatal("Unable to allocate register when one should be available.");
        }
        value->reg_alloc = alloc_reg(reg, type);
        state->reg_available[reg] = false;
        //  add interval to active, sorted by increasing end point
        state->active[state->num_active] = value;
        state->num_active++;
    }

}

// https://web.cs.ucla.edu/~palsberg/course/cs132/linearscan.pdf
void ir_allocate_registers() {
    // Recalculate indices before register allocation. Needed because previous steps can insert values into the middle of the list without updating the index values.
    ir_recalculate_indices();

    int spill_index = 0;

    register_allocation_state_t gpr_state;
    memset(&gpr_state, 0, sizeof(gpr_state));
    gpr_state.num_total_regs = get_num_gprs();
    gpr_state.num_regs = 0;
    for (int i = 0; i < get_num_preserved_gprs(); i++) {
        gpr_state.reg_available[get_preserved_gprs()[i]] = true;
        gpr_state.num_regs++;
    }

    register_allocation_state_t fgr_state;
    memset(&fgr_state, 0, sizeof(fgr_state));
    fgr_state.num_total_regs = get_num_fgrs();
    fgr_state.num_regs = get_num_fgrs(); // all are available
    for (int i = 0; i < fgr_state.num_total_regs; i++) {
        fgr_state.reg_available[i] = true;
    }

    // TODO: replace last_use calculations with a single backwards pass instead of the worst-case O(n^2) algorithm of calling value_lifetime()
    ir_instruction_t* value = ir_context.ir_cache_head;
    while (value != NULL) {
        // Sort in order of increasing last usage
        sort_active(&gpr_state);
        sort_active(&fgr_state);

        expire_old_intervals(&gpr_state, value);
        expire_old_intervals(&fgr_state, value);

        memset(&value->reg_alloc, 0, sizeof(value->reg_alloc));
        ir_register_type_t allocation_type = get_required_register_type(value);
        if (allocation_type != REGISTER_TYPE_NONE) {
            if (allocation_type == REGISTER_TYPE_GPR) {
                allocate_register(value, allocation_type, &spill_index, &gpr_state);
            } else if (allocation_type == REGISTER_TYPE_FGR_32 || allocation_type == REGISTER_TYPE_FGR_64) {
                allocate_register(value, allocation_type, &spill_index, &fgr_state);
            } else {
                logfatal("Unknown register allocation type!");
            }
        }
        value = value->next;
    }
}
