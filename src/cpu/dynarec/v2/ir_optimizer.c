#include <log.h>
#include <string.h>
#include "ir_optimizer.h"
#include "ir_context.h"

INLINE bool is_constant(ir_instruction_t* instr) {
    return instr->type == IR_SET_CONSTANT;
}

INLINE bool binop_constant(ir_instruction_t* instr) {
    return is_constant(&ir_context.ir_cache[instr->bin_op.operand1]) && is_constant(&ir_context.ir_cache[instr->bin_op.operand2]);
}

u64 const_to_u64(ir_instruction_t* constant) {
    switch (constant->set_constant.type) {
        case VALUE_TYPE_S16:
            return (s64)constant->set_constant.value_s16;
        case VALUE_TYPE_U16:
            return constant->set_constant.value_u16;
        case VALUE_TYPE_S32:
            logfatal("Unimplemented");
            //return (s64)constant->set_constant.value_s32;
        case VALUE_TYPE_U32:
            logfatal("Unimplemented");
            //return constant->set_constant.value_u32;
        case VALUE_TYPE_64:
            return constant->set_constant.value_64;
    }
}

void ir_optimize_constant_propagation() {
    for (int i = 0; i < ir_context.ir_cache_index; i++) {
        ir_instruction_t* instr = &ir_context.ir_cache[i];

        switch (instr->type) {
            // Unable to be optimized further
            case IR_NOP:
            case IR_SET_CONSTANT:
            case IR_STORE:
            case IR_LOAD:
            case IR_SET_BLOCK_EXIT_PC:
                break;

            case IR_OR:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(&ir_context.ir_cache[instr->bin_op.operand1]);
                    u64 operand2 = const_to_u64(&ir_context.ir_cache[instr->bin_op.operand2]);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = operand1 | operand2;
                }
                break;

            case IR_AND:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(&ir_context.ir_cache[instr->bin_op.operand1]);
                    u64 operand2 = const_to_u64(&ir_context.ir_cache[instr->bin_op.operand2]);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = operand1 & operand2;
                }
                // TODO: check if one operand is constant, if it's zero, and replace with a const zero here
                break;

            case IR_ADD:
                if (binop_constant(instr)) {
                    u64 operand1 = const_to_u64(&ir_context.ir_cache[instr->bin_op.operand1]);
                    u64 operand2 = const_to_u64(&ir_context.ir_cache[instr->bin_op.operand2]);

                    instr->type = IR_SET_CONSTANT;
                    instr->set_constant.type = VALUE_TYPE_64;
                    instr->set_constant.value_64 = operand1 + operand2;
                }
                break;

            case IR_MASK_AND_CAST:
                if (is_constant(&ir_context.ir_cache[instr->mask_and_cast.operand])) {
                    u64 value = const_to_u64(&ir_context.ir_cache[instr->mask_and_cast.operand]);
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

            case IR_CHECK_CONDITION:
                break;
        }
    }
}

void ir_optimize_eliminate_dead_code() {
    static bool ir_result_used[IR_CACHE_SIZE];
    memset(ir_result_used, 0, sizeof(bool) * IR_CACHE_SIZE);

    for (int i = 1; i < 32; i++) {
        int ssa_index = ir_context.guest_gpr_to_value[i];
        if (ssa_index >= 0) {
            printf("v%d is used for the value of r%d after the block!\n", ssa_index, i);
            ir_result_used[ssa_index] = true;
        }
    }

    for (int i = ir_context.ir_cache_index - 1; i >= 0; i--) {
        ir_instruction_t* instr = &ir_context.ir_cache[i];

        switch (ir_context.ir_cache[i].type) {
            // Never eliminated
            case IR_STORE:
                ir_result_used[instr->store.address] = true;
                ir_result_used[instr->store.value] = true;
                ir_result_used[i] = true;
                break;
            case IR_LOAD:
                ir_result_used[instr->load.address] = true;
                ir_result_used[i] = true;
                break;
            case IR_SET_BLOCK_EXIT_PC:
                ir_result_used[instr->set_exit_pc.condition] = true;
                ir_result_used[instr->set_exit_pc.pc_if_true] = true;
                ir_result_used[instr->set_exit_pc.pc_if_false] = true;
                ir_result_used[i] = true;
                break;

            // Bin ops
            case IR_OR:
            case IR_AND:
            case IR_ADD:
                if (ir_result_used[i]) {
                    ir_result_used[instr->bin_op.operand1] = true;
                    ir_result_used[instr->bin_op.operand2] = true;
                }
                break;
            case IR_MASK_AND_CAST:
                if (ir_result_used[i]) {
                    ir_result_used[instr->mask_and_cast.operand] = true;
                }
                break;
            case IR_CHECK_CONDITION:
                if (ir_result_used[i]) {
                    ir_result_used[instr->check_condition.operand1] = true;
                    ir_result_used[instr->check_condition.operand2] = true;
                }
                break;

            // No dependencies
            case IR_NOP:
                break;
            case IR_SET_CONSTANT:
                break;
        }
    }

    for (int i = 0; i < ir_context.ir_cache_index; i++) {
        if (!ir_result_used[i]) {
            ir_context.ir_cache[i].type = IR_NOP;
        }
    }
}
