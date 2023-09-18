#include <log.h>
#include <r4300i.h>
#include <mem/n64bus.h>
#include "ir_context.h"
#include "ir_optimizer.h"

ir_context_t ir_context;

void ir_context_reset() {
    for (int i = 0; i < 64; i++) {
        ir_context.guest_reg_to_value[i] = NULL;
        if (IR_IS_GPR(i)) {
            ir_context.guest_reg_to_reg_type[i] = REGISTER_TYPE_GPR;
        } else if (IR_IS_FGR(i)) {
            ir_context.guest_reg_to_reg_type[i] = REGISTER_TYPE_NONE; // Float size is unknown at this point
        } else {
            logfatal("Unknown or out of bounds guest register: %d", i);
        }
    }

    memset(ir_context.ir_cache, 0, sizeof(ir_instruction_t) * IR_CACHE_SIZE);

    ir_context.ir_cache[0].type = IR_SET_CONSTANT;
    ir_context.ir_cache[0].set_constant.type = VALUE_TYPE_U64;
    ir_context.ir_cache[0].set_constant.value_u64 = 0;
    ir_context.guest_reg_to_value[0] = &ir_context.ir_cache[0];

    ir_context.ir_cache_index = 1;

    ir_context.ir_cache_head = &ir_context.ir_cache[0];
    ir_context.ir_cache_tail = &ir_context.ir_cache[0];

    ir_context.block_start_virtual = 0;
    ir_context.block_start_physical = 0;

    ir_context.block_ended = false;

    ir_context.block_end_pc_compiled = false;
    ir_context.block_end_pc_ir_emitted = false;

    ir_context.cp1_checked = false;
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
        case VALUE_TYPE_U64:
            return "U64";
        case VALUE_TYPE_S64:
            return "S64";
    }
}

const char* reg_type_to_str(ir_register_type_t type) {
    switch (type) {
        case REGISTER_TYPE_NONE:
            return "NONE";
        case REGISTER_TYPE_GPR:
            return "GPR";
        case REGISTER_TYPE_FGR_32:
            return "FGR32";
        case REGISTER_TYPE_FGR_64:
            return "FGR64";
        case REGISTER_TYPE_VPR:
            return "VPR";
    }
}

const char* float_type_to_str(ir_float_value_type_t type) {
    switch (type) {
        case FLOAT_VALUE_TYPE_INVALID:
            return "INVALID";
        case FLOAT_VALUE_TYPE_WORD:
            return "WORD";
        case FLOAT_VALUE_TYPE_LONG:
            return "LONG";
        case FLOAT_VALUE_TYPE_SINGLE:
            return "SINGLE";
        case FLOAT_VALUE_TYPE_DOUBLE:
            return "DOUBLE";
    }
}

const char* cond_to_str(ir_condition_t condition) {
    switch (condition) {
        case CONDITION_NOT_EQUAL:
            return "!=";
        case CONDITION_EQUAL:
            return "==";
        case CONDITION_LESS_THAN_SIGNED:
        case CONDITION_LESS_THAN_UNSIGNED:
            return "<";
        case CONDITION_GREATER_THAN_SIGNED:
        case CONDITION_GREATER_THAN_UNSIGNED:
            return ">";
        case CONDITION_LESS_OR_EQUAL_TO_SIGNED:
        case CONDITION_LESS_OR_EQUAL_TO_UNSIGNED:
            return "<=";
        case CONDITION_GREATER_OR_EQUAL_TO_SIGNED:
        case CONDITION_GREATER_OR_EQUAL_TO_UNSIGNED:
            return ">=";
    }
}

const char* float_cond_to_str(ir_float_condition_t condition) {
    switch (condition) {
        case CONDITION_FLOAT_LT:
            return "<";
        case CONDITION_FLOAT_LE:
            return "<=";
        case CONDITION_FLOAT_EQ:
            return "==";
        case CONDITION_FLOAT_NGE:
            return "!>=";
        case CONDITION_FLOAT_NGT:
            return "!>";
        case CONDITION_FLOAT_UN:
            return "UN";
    }
    logfatal("Did not match any cases");
}

const char* rsp_lwc2_instruction_to_str(rsp_lwc2_instruction_t type) {
    switch (type) {
        case IR_RSP_LWC2_LDV:
            return "ldv";
        case IR_RSP_LWC2_LQV:
            return "lqv";
        case IR_RSP_LWC2_LBV:
            return "lbv";
        case IR_RSP_LWC2_LFV:
            return "lfv";
        case IR_RSP_LWC2_LHV:
            return "lhv";
        case IR_RSP_LWC2_LLV:
            return "llv";
        case IR_RSP_LWC2_LPV:
            return "lpv";
        case IR_RSP_LWC2_LRV:
            return "lrv";
        case IR_RSP_LWC2_LSV:
            return "lsv";
        case IR_RSP_LWC2_LTV:
            return "ltv";
        case IR_RSP_LWC2_LUV:
            return "luv";
        }
}

const char* rsp_swc2_instruction_to_str(rsp_swc2_instruction_t type) {
    switch (type) {
        case IR_RSP_SWC2_SBV:
            return "sbv";
        case IR_RSP_SWC2_SSV:
            return "ssv";
        case IR_RSP_SWC2_SLV:
            return "slv";
        case IR_RSP_SWC2_SDV:
            return "sdv";
        case IR_RSP_SWC2_SQV:
            return "sqv";
        case IR_RSP_SWC2_SRV:
            return "srv";
        case IR_RSP_SWC2_SPV:
            return "spv";
        case IR_RSP_SWC2_SUV:
            return "suv";
        case IR_RSP_SWC2_SHV:
            return "shv";
        case IR_RSP_SWC2_SFV:
            return "sfv";
        case IR_RSP_SWC2_SWV:
            return "swv";
        case IR_RSP_SWC2_STV:
            return "stv";
        }
}

void ir_instr_to_string(ir_instruction_t* instr, char* buf, size_t buf_size) {

    if (instr->type != IR_STORE
        && instr->type != IR_RSP_SWC2
        && instr->type != IR_SET_COND_BLOCK_EXIT_PC
        && instr->type != IR_SET_BLOCK_EXIT_PC
        && instr->type != IR_NOP
        && instr->type != IR_FLUSH_GUEST_REG
        && instr->type != IR_FLOAT_CHECK_CONDITION) {
        int written = snprintf(buf, buf_size, "v%d = ", instr->index);
        buf += written;
        buf_size -= written;
    }

    switch (instr->type) {
        case IR_NOP:
            break;
        case IR_SET_CONSTANT:
            switch (instr->set_constant.type) {
                case VALUE_TYPE_U8:
                    snprintf(buf, buf_size, "0x%04X ;%u (u8)", (u16)instr->set_constant.value_u8, instr->set_constant.value_u8);
                    break;
                case VALUE_TYPE_S8:
                    snprintf(buf, buf_size, "0x%04X ;%d (s8)", (u16)instr->set_constant.value_s8, instr->set_constant.value_s8);
                    break;
                case VALUE_TYPE_S16:
                    snprintf(buf, buf_size, "0x%04X ;%d (s16)", (u16)instr->set_constant.value_s16, instr->set_constant.value_s16);
                    break;
                case VALUE_TYPE_U16:
                    snprintf(buf, buf_size, "0x%04X ;%u (u16)", instr->set_constant.value_u16, instr->set_constant.value_u16);
                    break;
                case VALUE_TYPE_S32:
                    snprintf(buf, buf_size, "0x%08X ;%d (s32)", (u32)instr->set_constant.value_s32, instr->set_constant.value_s32);
                    break;
                case VALUE_TYPE_U32:
                    snprintf(buf, buf_size, "0x%08X ;%u (u32)", instr->set_constant.value_u32, instr->set_constant.value_u32);
                    break;
                case VALUE_TYPE_U64:
                    snprintf(buf, buf_size, "0x%016" PRIX64 " ;%" PRIu64 " (u64)", instr->set_constant.value_u64, instr->set_constant.value_u64);
                    break;
                case VALUE_TYPE_S64:
                    snprintf(buf, buf_size, "0x%016" PRIX64 " ;%" PRId64 " (u64)", instr->set_constant.value_s64, instr->set_constant.value_s64);
                    break;
            }
            break;
        case IR_SET_FLOAT_CONSTANT:
            snprintf(buf, buf_size, "FLOAT_CONSTANT");
            break;
        case IR_OR:
            snprintf(buf, buf_size, "v%d | v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_XOR:
            snprintf(buf, buf_size, "v%d ^ v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_AND:
            snprintf(buf, buf_size, "v%d & v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_NOT:
            snprintf(buf, buf_size, "~v%d", instr->unary_op.operand->index);
            break;
        case IR_ADD:
            snprintf(buf, buf_size, "v%d + v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_STORE:
            snprintf(buf, buf_size, "STORE(type = %s, address = v%d, value = v%d)", val_type_to_str(instr->store.type), instr->store.address->index, instr->store.value->index);
            break;
        case IR_LOAD:
            snprintf(buf, buf_size, "LOAD(type = %s, address = v%d)", val_type_to_str(instr->load.type), instr->load.address->index);
            break;
        case IR_GET_PTR:
            snprintf(buf, buf_size, "GETPTR(type = %s, ptr = %" PRIx64 ")", val_type_to_str(instr->set_ptr.type), instr->set_ptr.ptr);
            break;
        case IR_SET_PTR:
            snprintf(buf, buf_size, "SETPTR(type = %s, ptr = %" PRIx64 ", value = v%d)", val_type_to_str(instr->set_ptr.type), instr->set_ptr.ptr, instr->set_ptr.value->index);
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
        case IR_FLOAT_CHECK_CONDITION:
            snprintf(buf, buf_size, "fcr31.compare = v%d %s v%d",
                     instr->float_check_condition.operand1->index,
                     float_cond_to_str(instr->float_check_condition.condition),
                     instr->float_check_condition.operand2->index);
            break;
        case IR_SET_BLOCK_EXIT_PC:
            snprintf(buf, buf_size, "set_block_exit(v%d)", instr->unary_op.operand->index);
            break;
        case IR_SET_COND_BLOCK_EXIT_PC:
            snprintf(buf, buf_size, "set_block_exit(v%d, if_true = v%d, if_false = v%d)", instr->set_cond_exit_pc.condition->index, instr->set_cond_exit_pc.pc_if_true->index, instr->set_cond_exit_pc.pc_if_false->index);
            break;
        case IR_TLB_LOOKUP:
            snprintf(buf, buf_size, "tlb_lookup(v%d)", instr->tlb_lookup.virtual_address->index);
            break;
        case IR_LOAD_GUEST_REG:
            if (instr->load_guest_reg.guest_reg < 32) {
                snprintf(buf, buf_size, "guest_gpr[%d]", instr->load_guest_reg.guest_reg);
            } else {
                snprintf(buf, buf_size, "guest_fgr[%d]", instr->load_guest_reg.guest_reg - 32);
            }
            break;
        case IR_FLUSH_GUEST_REG:
            if (instr->flush_guest_reg.guest_reg < 32) {
                snprintf(buf, buf_size, "guest_gpr[%d] = v%d", instr->flush_guest_reg.guest_reg, instr->flush_guest_reg.value->index);
            } else {
                snprintf(buf, buf_size, "guest_fgr[%d] = v%d", instr->flush_guest_reg.guest_reg - 32, instr->flush_guest_reg.value->index);
            }
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
        case IR_COND_BLOCK_EXIT:
            snprintf(buf, buf_size, "exit_block_if(v%d)", instr->cond_block_exit.condition->index);
            break;
        case IR_MULTIPLY:
            snprintf(buf, buf_size, "(%s)v%d * (%s)v%d", val_type_to_str(instr->mult_div.mult_div_type), instr->mult_div.operand1->index, val_type_to_str(instr->mult_div.mult_div_type), instr->mult_div.operand2->index);
            break;
        case IR_DIVIDE:
            snprintf(buf, buf_size, "(%s)v%d / (%s)v%d", val_type_to_str(instr->mult_div.mult_div_type), instr->mult_div.operand1->index, val_type_to_str(instr->mult_div.mult_div_type), instr->mult_div.operand2->index);
            break;
        case IR_SUB:
            snprintf(buf, buf_size, "v%d - v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_ERET:
            snprintf(buf, buf_size, "eret()");
            break;
        case IR_CALL: {
            int written = snprintf(buf, buf_size, "call_func(func = %" PRIx64 ", args = { ", instr->call.function);
            buf += written;
            buf_size -= written;
            for (int i = 0; i < instr->call.num_args; i++) {
                written = snprintf(buf, buf_size, "v%d ", instr->call.arguments[i]->index);
                buf += written;
                buf_size -= written;
            }
            snprintf(buf, buf_size, "}");
            break;
        }
        case IR_MOV_REG_TYPE:
            snprintf(buf, buf_size, "to_type(v%d, reg_type = %s, size = %s)", instr->mov_reg_type.value->index, reg_type_to_str(instr->mov_reg_type.new_type), val_type_to_str(instr->mov_reg_type.size));
            break;
        case IR_FLOAT_CONVERT:
            snprintf(buf, buf_size, "float_convert(v%d, from = %s, to = %s)", instr->float_convert.value->index, float_type_to_str(instr->float_convert.from_type), float_type_to_str(instr->float_convert.to_type));
            break;
        case IR_FLOAT_DIVIDE:
            snprintf(buf, buf_size, "(%s)v%d / (%s)v%d", float_type_to_str(instr->float_bin_op.format), instr->float_bin_op.operand1->index, float_type_to_str(instr->float_bin_op.format), instr->float_bin_op.operand2->index);
            break;
        case IR_FLOAT_MULTIPLY:
            snprintf(buf, buf_size, "(%s)v%d * (%s)v%d", float_type_to_str(instr->float_bin_op.format), instr->float_bin_op.operand1->index, float_type_to_str(instr->float_bin_op.format), instr->float_bin_op.operand2->index);
            break;
        case IR_FLOAT_ADD:
            snprintf(buf, buf_size, "(%s)v%d + (%s)v%d", float_type_to_str(instr->float_bin_op.format), instr->float_bin_op.operand1->index, float_type_to_str(instr->float_bin_op.format), instr->float_bin_op.operand2->index);
            break;
        case IR_FLOAT_SUB:
            snprintf(buf, buf_size, "(%s)v%d - (%s)v%d", float_type_to_str(instr->float_bin_op.format), instr->float_bin_op.operand1->index, float_type_to_str(instr->float_bin_op.format), instr->float_bin_op.operand2->index);
            break;
        case IR_FLOAT_NEG:
            snprintf(buf, buf_size, "-v%d", instr->float_unary_op.operand->index);
            break;
        case IR_FLOAT_SQRT:
            snprintf(buf, buf_size, "sqrt(v%d)", instr->float_unary_op.operand->index);
            break;
        case IR_FLOAT_ABS:
            snprintf(buf, buf_size, "abs(v%d)", instr->float_unary_op.operand->index);
            break;
        case IR_INTERPRETER_FALLBACK:
            switch (instr->interpreter_fallback.type) {
                case INTERPRETER_FALLBACK_FOR_INSTRUCTIONS:
                    snprintf(buf, buf_size, "interpreter_fallback(instructions = %d", instr->interpreter_fallback.for_instructions);
                    break;
                case INTERPRETER_FALLBACK_UNTIL_NO_BRANCH:
                    snprintf(buf, buf_size, "interpreter_fallback(all_branches_resolved)");
                    break;
            }
            break;
        case IR_RSP_LWC2:
            snprintf(buf, buf_size, "lwc2_%s(v%d, old_value = v%d, element = %d)", rsp_lwc2_instruction_to_str(instr->rsp_lwc2.type), instr->rsp_lwc2.addr->index, instr->rsp_lwc2.old_value->index, instr->rsp_lwc2.element);
            break;
        case IR_RSP_SWC2:
            snprintf(buf, buf_size, "swc2_%s(address = v%d, value = v%d, element = %d)", rsp_swc2_instruction_to_str(instr->rsp_swc2.type), instr->rsp_swc2.addr->index, instr->rsp_swc2.value->index, instr->rsp_swc2.element);
            break;
        case IR_VPR_INSERT:
            snprintf(buf, buf_size, "vpr_insert(old_value = v%d, to_insert = v%d, offset = %d, type = %s)",
                     instr->vpr_insert.old_value->index,
                     instr->vpr_insert.value_to_insert->index,
                     instr->vpr_insert.byte_offset,
                     val_type_to_str(instr->vpr_insert.value_type));
            break;
    }
}

void print_ir_block() {
    ir_instruction_t* instr = ir_context.ir_cache_head;
    while (instr != NULL) {
        static char buf[100];
        ir_instr_to_string(instr, buf, 100);
        printf("%s\n", buf);

        instr = instr->next;
    }
}

bool instr_exception_possible(ir_instruction_t* instr) {
    switch (instr->type) {
        case IR_NOP:
        case IR_SET_CONSTANT:
        case IR_SET_FLOAT_CONSTANT:
        case IR_OR:
        case IR_XOR:
        case IR_AND:
        case IR_SUB:
        case IR_NOT:
        case IR_ADD:
        case IR_SHIFT:
        case IR_STORE:
        case IR_LOAD:
        case IR_GET_PTR:
        case IR_SET_PTR:
        case IR_MASK_AND_CAST:
        case IR_CHECK_CONDITION:
        case IR_SET_COND_BLOCK_EXIT_PC:
        case IR_SET_BLOCK_EXIT_PC:
        case IR_LOAD_GUEST_REG:
        case IR_FLUSH_GUEST_REG:
        case IR_MULTIPLY:
        case IR_DIVIDE:
        case IR_ERET:
        case IR_CALL:
        case IR_MOV_REG_TYPE:
        case IR_FLOAT_CONVERT:
        case IR_FLOAT_MULTIPLY:
        case IR_FLOAT_DIVIDE:
        case IR_FLOAT_ADD:
        case IR_FLOAT_SUB:
        case IR_FLOAT_SQRT:
        case IR_FLOAT_ABS:
        case IR_FLOAT_NEG:
        case IR_FLOAT_CHECK_CONDITION:
        case IR_INTERPRETER_FALLBACK:
        // No RSP instructions ever throw exceptions
        case IR_RSP_LWC2:
        case IR_RSP_SWC2:
        case IR_VPR_INSERT:
            return false;

        case IR_COND_BLOCK_EXIT:
            return true;
        case IR_TLB_LOOKUP:
            if (is_constant(instr->tlb_lookup.virtual_address)) {
                u64 vaddr = const_to_u64(instr->tlb_lookup.virtual_address);
                if (is_tlb(vaddr)) {
                    return true;
                } else {
                    return false;
                }
            }
            return true;
    }
}

void update_guest_reg_mapping(u8 guest_reg, ir_instruction_t* value) {
    if (guest_reg > 0) {
        if (IR_IS_GPR(guest_reg)) {
            ir_context.guest_reg_to_value[guest_reg] = value;
        } else if (IR_IS_FGR(guest_reg)) {
            ir_register_type_t new_type = REGISTER_TYPE_NONE;


            switch (value->type) {
                case IR_SET_CONSTANT:
                case IR_NOP:
                case IR_OR:
                case IR_XOR:
                case IR_AND:
                case IR_SUB:
                case IR_NOT:
                case IR_ADD:
                case IR_SHIFT:
                case IR_STORE:
                case IR_GET_PTR:
                case IR_SET_PTR:
                case IR_MASK_AND_CAST:
                case IR_CHECK_CONDITION:
                case IR_FLOAT_CHECK_CONDITION:
                case IR_SET_COND_BLOCK_EXIT_PC:
                case IR_SET_BLOCK_EXIT_PC:
                case IR_COND_BLOCK_EXIT:
                case IR_TLB_LOOKUP:
                case IR_FLUSH_GUEST_REG:
                case IR_MULTIPLY:
                case IR_DIVIDE:
                case IR_ERET:
                case IR_CALL:
                case IR_INTERPRETER_FALLBACK:
                case IR_RSP_SWC2:
                    logfatal("Unsupported IR instruction assigned to FPU reg");

                case IR_LOAD_GUEST_REG:
                    new_type = value->load_guest_reg.guest_reg_type;
                    break;

                case IR_LOAD:
                    new_type = value->load.reg_type;
                    break;

                case IR_MOV_REG_TYPE:
                    unimplemented(value->mov_reg_type.new_type != REGISTER_TYPE_FGR_32 &&
                                  value->mov_reg_type.new_type != REGISTER_TYPE_FGR_64, "non-float mapped to FGR!");
                    new_type = value->mov_reg_type.new_type;
                    break;

                case IR_FLOAT_CONVERT:
                    new_type = float_val_to_reg_type(value->float_convert.to_type);
                    break;

                // Float bin ops
                case IR_FLOAT_DIVIDE:
                case IR_FLOAT_MULTIPLY:
                case IR_FLOAT_ADD:
                case IR_FLOAT_SUB:
                    new_type = float_val_to_reg_type(value->float_bin_op.format);
                    break;

                // Float unary ops
                case IR_FLOAT_ABS:
                case IR_FLOAT_NEG:
                case IR_FLOAT_SQRT:
                    new_type = float_val_to_reg_type(value->float_unary_op.format);
                    break;

                case IR_SET_FLOAT_CONSTANT:
                    new_type = float_val_to_reg_type(value->set_float_constant.format);
                    break;
                
                // RSP vector operations
                case IR_RSP_LWC2:
                case IR_VPR_INSERT:
                    new_type = REGISTER_TYPE_VPR;
                    break;
            }

            ir_context.guest_reg_to_value[guest_reg] = value;
            ir_context.guest_reg_to_reg_type[guest_reg] = new_type;
        }
    }
}

ir_instruction_t* allocate_ir_instruction(ir_instruction_t instruction) {
    int index = ir_context.ir_cache_index++;
    ir_context.ir_cache[index] = instruction;
    ir_context.ir_cache[index].index = index;
    ir_context.ir_cache[index].dead_code = true; // Will be marked false at the dead code elimination stage
    ir_context.ir_cache[index].reg_alloc.allocated = false;
    ir_context.ir_cache[index].reg_alloc.host_reg = -1;
    ir_context.ir_cache[index].reg_alloc.spilled = false;
    ir_context.ir_cache[index].last_use = -1;
    return &ir_context.ir_cache[index];
}

ir_instruction_t* append_ir_instruction(ir_instruction_t instruction, int index, u8 guest_reg) {
    instruction.flush_info.num_regs = 0;
    if (instr_exception_possible(&instruction)) {
        for (int i = 1; i < 64; i++) {
            ir_instruction_t* gpr_value = ir_context.guest_reg_to_value[i];
            if (gpr_value) {
                // If it's just a load, no need to flush it back as it has not been modified
                if (gpr_value->type != IR_LOAD_GUEST_REG || gpr_value->load_guest_reg.guest_reg != i) {
                    ir_instruction_flush_t* flush = &instruction.flush_info.regs[instruction.flush_info.num_regs++];
                    flush->guest_reg = i;
                    flush->item = gpr_value;
                }
            }
        }
    }

    ir_instruction_t* allocation = allocate_ir_instruction(instruction);

    allocation->block_length = index + 1;

    allocation->next = NULL;
    allocation->prev = ir_context.ir_cache_tail;

    ir_context.ir_cache_tail->next = allocation;
    ir_context.ir_cache_tail = allocation;

    update_guest_reg_mapping(guest_reg, allocation);
    return allocation;
}

ir_instruction_t* insert_ir_instruction(ir_instruction_t* after, ir_instruction_t instruction) {
    if (after == NULL) {
        logfatal("insert_ir_instruction with null 'after'");
    }
    ir_instruction_t* old_next = after->next;
    if (old_next == NULL) {
        // Inserting at the end
        return append_ir_instruction(instruction, -1, NO_GUEST_REG);
    } else {
        if (instr_exception_possible(&instruction)) {
            logfatal("Cannot insert an instruction that can cause an exception!");
        }
        instruction.flush_info.num_regs = 0;
        ir_instruction_t* allocation = allocate_ir_instruction(instruction);

        after->next = allocation;

        allocation->prev = after;
        allocation->next = old_next;

        old_next->prev = allocation;
        return allocation;
    }
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
        case VALUE_TYPE_U64:
            is_zero = value.value_u64 == 0;
            break;
        case VALUE_TYPE_S64:
            is_zero = value.value_s64 == 0;
            break;
    }
    if (is_zero) {
        update_guest_reg_mapping(guest_reg, ir_context.ir_cache_head);
        return ir_context.ir_cache_head;
    }

    ir_instruction_t instruction;
    instruction.type = IR_SET_CONSTANT;
    instruction.set_constant = value;

    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_load_guest_gpr(u8 guest_reg) {
    if (IR_IS_GPR(guest_reg)) {
        if (ir_context.guest_reg_to_value[guest_reg] != NULL) {
            return ir_context.guest_reg_to_value[guest_reg];
        }

        ir_instruction_t instruction;
        instruction.type = IR_LOAD_GUEST_REG;
        instruction.load_guest_reg.guest_reg = guest_reg;
        instruction.load_guest_reg.guest_reg_type = REGISTER_TYPE_GPR;

        return append_ir_instruction(instruction, -1, guest_reg);
    } else if (IR_IS_FGR(guest_reg)) {
        logfatal("Loading FGR with ir_emit_load_guest_gpr(), use ir_emit_load_guest_fgr()");
    } else {
        logfatal("Loading unknown (or out of range) guest register %d", guest_reg);
    }
}

ir_instruction_t* ir_emit_load_guest_fgr(u8 guest_fgr, ir_float_value_type_t type) {
    unimplemented(ir_context.target != COMPILER_TARGET_CPU, "Loading FGR when not targeting CPU!");
    if (IR_IS_GPR(guest_fgr)) {
        logfatal("Loading GPR with ir_emit_load_guest_fgr(), use ir_emit_load_guest_gpr()");
    } else if (IR_IS_FGR(guest_fgr)) {
        ir_instruction_t* prev = ir_context.guest_reg_to_value[guest_fgr];
        bool should_reload =
                // No cached value -> should reload
                prev == NULL
                // Reg type differs from what we need -> should reload
                || ir_context.guest_reg_to_reg_type[guest_fgr] != float_val_to_reg_type(type);

        if (!should_reload) {
            return ir_context.guest_reg_to_value[guest_fgr];
        } else {
            if (prev != NULL) {
                ir_emit_flush_guest_reg(prev, prev, guest_fgr);
            }
            ir_instruction_t instruction;
            instruction.type = IR_LOAD_GUEST_REG;
            instruction.load_guest_reg.guest_reg = guest_fgr;
            ir_register_type_t new_type = float_val_to_reg_type(type);
            instruction.load_guest_reg.guest_reg_type = new_type;
            ir_context.guest_reg_to_reg_type[guest_fgr] = new_type;
            return append_ir_instruction(instruction, -1, IR_FGR(guest_fgr));
        }
    } else {
        logfatal("Loading unknown (or out of range) guest register %d", guest_fgr);
    }
}

ir_instruction_t* ir_emit_load_guest_vpr(u8 guest_vpr) {
    unimplemented(ir_context.target != COMPILER_TARGET_RSP, "Loading VPR when not targeting RSP!");
    if (IR_IS_VPR(guest_vpr)) {
        if (ir_context.guest_reg_to_value[guest_vpr] != NULL) {
            return ir_context.guest_reg_to_value[guest_vpr];
        }

        ir_instruction_t instruction;
        instruction.type = IR_LOAD_GUEST_REG;
        instruction.load_guest_reg.guest_reg = guest_vpr;
        instruction.load_guest_reg.guest_reg_type = REGISTER_TYPE_VPR;
        return append_ir_instruction(instruction, -1, guest_vpr);
    } else {
        logfatal("ir_emit_load_guest_vpr with non-vpr argument");
    }
}

ir_instruction_t* ir_emit_flush_guest_reg(ir_instruction_t* last_usage, ir_instruction_t* value, u8 guest_reg) {
    if (last_usage == NULL) {
        logfatal("ir_emit_flush_guest_reg with null last_usage");
    }

    if (guest_reg == 0) {
        logfatal("Should never flush r0");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLUSH_GUEST_REG;
    instruction.flush_guest_reg.guest_reg = guest_reg;
    instruction.flush_guest_reg.value = value;

    return insert_ir_instruction(last_usage, instruction);
}

ir_instruction_t* ir_emit_or(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_OR;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_xor(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_XOR;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_not(ir_instruction_t* operand, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_NOT;
    instruction.unary_op.operand = operand;

    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_boolean_not(ir_instruction_t* operand, u8 guest_reg) {
    ir_instruction_t* mask = ir_emit_set_constant_u16(1, NO_GUEST_REG);
    ir_instruction_t* notted = ir_emit_not(operand, NO_GUEST_REG);
    return ir_emit_and(notted, mask, guest_reg);
}

ir_instruction_t* ir_emit_and(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_AND;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_sub(ir_instruction_t* minuend, ir_instruction_t* subtrahend, ir_value_type_t type, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_SUB;
    instruction.bin_op.operand1 = minuend;
    instruction.bin_op.operand2 = subtrahend;
    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_add(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_ADD;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_shift(ir_instruction_t* operand, ir_instruction_t* amount, ir_value_type_t value_type, ir_shift_direction_t direction, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_SHIFT;
    instruction.shift.operand = operand;
    instruction.shift.amount = amount;
    instruction.shift.type = value_type;
    instruction.shift.direction = direction;

    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_store(ir_value_type_t type, ir_instruction_t* address, ir_instruction_t* value) {
    ir_instruction_t instruction;
    instruction.type = IR_STORE;
    instruction.store.type = type;
    instruction.store.address = address;
    instruction.store.value = value;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_load(ir_value_type_t type, ir_instruction_t* address, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_LOAD;
    instruction.load.type = type;
    instruction.load.address = address;
    if (IR_IS_GPR(guest_reg) || guest_reg == NO_GUEST_REG) {
        instruction.load.reg_type = REGISTER_TYPE_GPR;
    } else if (IR_IS_FGR(guest_reg)) {
        switch (type) {
            CASE_SIZE_8:
            CASE_SIZE_16:
                logfatal("Invalid FGR size");
            CASE_SIZE_32:
                instruction.load.reg_type = REGISTER_TYPE_FGR_32;
                break;
            CASE_SIZE_64:
                instruction.load.reg_type = REGISTER_TYPE_FGR_64;
                break;
        }
    } else {
        logfatal("Unknown guest reg type");
    }
    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_get_ptr(ir_value_type_t type, void* ptr, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_GET_PTR;
    instruction.get_ptr.type = type;
    instruction.get_ptr.ptr = (uintptr_t)ptr;
    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_set_ptr(ir_value_type_t type, void* ptr, ir_instruction_t* value) {
    ir_instruction_t instruction;
    instruction.type = IR_SET_PTR;
    instruction.set_ptr.type = type;
    instruction.set_ptr.ptr = (uintptr_t)ptr;
    instruction.set_ptr.value = value;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_mask_and_cast(ir_instruction_t* operand, ir_value_type_t type, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_MASK_AND_CAST;
    instruction.mask_and_cast.type = type;
    instruction.mask_and_cast.operand = operand;
    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_check_condition(ir_condition_t condition, ir_instruction_t* operand1, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_CHECK_CONDITION;
    instruction.check_condition.condition = condition;
    instruction.check_condition.operand1 = operand1;
    instruction.check_condition.operand2 = operand2;
    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_conditional_set_block_exit_pc(ir_instruction_t* condition, ir_instruction_t* pc_if_true, ir_instruction_t* pc_if_false) {
    ir_context.block_end_pc_ir_emitted = true;
    ir_instruction_t instruction;
    instruction.type = IR_SET_COND_BLOCK_EXIT_PC;
    instruction.set_cond_exit_pc.condition = condition;
    instruction.set_cond_exit_pc.pc_if_true = pc_if_true;
    instruction.set_cond_exit_pc.pc_if_false = pc_if_false;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_conditional_block_exit_internal(int index, ir_instruction_t* condition, cond_block_exit_type_t type, cond_block_exit_info_t info) {
    ir_instruction_t instruction;
    instruction.type = IR_COND_BLOCK_EXIT;
    instruction.cond_block_exit.condition = condition;
    instruction.cond_block_exit.type = type;
    instruction.cond_block_exit.info = info;

    unimplemented(!ir_context.block_end_pc_ir_emitted && type == COND_BLOCK_EXIT_TYPE_NONE, "COND_BLOCK_EXIT_TYPE_NONE used when PC is unknown!");

    return append_ir_instruction(instruction, index, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_conditional_block_exit(int index, ir_instruction_t* condition) {
    cond_block_exit_info_t no_info = {0};
    return ir_emit_conditional_block_exit_internal(index, condition, COND_BLOCK_EXIT_TYPE_NONE, no_info);
}

ir_instruction_t* ir_emit_conditional_block_exit_exception(int index, ir_instruction_t* condition, dynarec_exception_t exception) {
    cond_block_exit_info_t info;
    info.exception = exception;
    return ir_emit_conditional_block_exit_internal(index, condition, COND_BLOCK_EXIT_TYPE_EXCEPTION, info);
}

ir_instruction_t* ir_emit_exception(int index, dynarec_exception_t exception) {
    ir_instruction_t* const_true = ir_emit_set_constant_u16(1, NO_GUEST_REG);
    return ir_emit_conditional_block_exit_exception(index, const_true, exception);
}

ir_instruction_t* ir_emit_conditional_block_exit_address(int index, ir_instruction_t* condition, ir_instruction_t* address) {
    cond_block_exit_info_t info;
    info.exit_pc = address;
    return ir_emit_conditional_block_exit_internal(index, condition, COND_BLOCK_EXIT_TYPE_ADDRESS, info);
}

ir_instruction_t* ir_emit_set_block_exit_pc(ir_instruction_t* address) {
    ir_context.block_end_pc_ir_emitted = true;
    ir_instruction_t instruction;
    instruction.type = IR_SET_BLOCK_EXIT_PC;
    instruction.unary_op.operand = address;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_interpreter_fallback_for_instructions(int num_instructions) {
    logfatal("Unimplemented: Fall back to interpreter for %d instructions", num_instructions);
}

ir_instruction_t* ir_emit_interpreter_fallback_until_no_delay_slot() {
    ir_instruction_t instruction;
    instruction.type = IR_INTERPRETER_FALLBACK;
    instruction.interpreter_fallback.type = INTERPRETER_FALLBACK_UNTIL_NO_BRANCH;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_tlb_lookup(int index, ir_instruction_t* virtual_address, u8 guest_reg, bus_access_t bus_access) {
    ir_instruction_t instruction;
    instruction.type = IR_TLB_LOOKUP;
    instruction.tlb_lookup.virtual_address = virtual_address;
    instruction.tlb_lookup.bus_access = bus_access;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_multiply(ir_instruction_t* multiplicand1, ir_instruction_t* multiplicand2, ir_value_type_t multiplicand_type) {
    ir_instruction_t instruction;
    instruction.type = IR_MULTIPLY;
    instruction.mult_div.operand1 = multiplicand1;
    instruction.mult_div.operand2 = multiplicand2;
    instruction.mult_div.mult_div_type = multiplicand_type;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_divide(ir_instruction_t* dividend, ir_instruction_t* divisor, ir_value_type_t divide_type) {
    ir_instruction_t instruction;
    instruction.type = IR_DIVIDE;
    instruction.mult_div.operand1 = dividend;
    instruction.mult_div.operand2 = divisor;
    instruction.mult_div.mult_div_type = divide_type;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_eret() {
    ir_context.block_end_pc_ir_emitted = true;
    ir_instruction_t instruction;
    instruction.type = IR_ERET;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

void ir_emit_call_0(uintptr_t function, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_CALL;
    instruction.call.save_result = guest_reg != NO_GUEST_REG;
    instruction.call.function = function;
    instruction.call.num_args = 0;
    append_ir_instruction(instruction, -1, guest_reg);
}

void ir_emit_call_1(uintptr_t function, ir_instruction_t* arg) {
    ir_instruction_t instruction;
    instruction.type = IR_CALL;
    instruction.call.save_result = false;
    instruction.call.function = function;
    instruction.call.num_args = 1;
    instruction.call.arguments[0] = arg;
    append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

void ir_emit_call_2(uintptr_t function, ir_instruction_t* arg1, ir_instruction_t* arg2) {
    ir_instruction_t instruction;
    instruction.type = IR_CALL;
    instruction.call.save_result = false;
    instruction.call.function = function;
    instruction.call.num_args = 2;
    instruction.call.arguments[0] = arg1;
    instruction.call.arguments[1] = arg2;
    append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

void ir_emit_call_3(uintptr_t function, ir_instruction_t* arg1, ir_instruction_t* arg2, ir_instruction_t* arg3) {
    ir_instruction_t instruction;
    instruction.type = IR_CALL;
    instruction.call.save_result = false;
    instruction.call.function = function;
    instruction.call.num_args = 3;
    instruction.call.arguments[0] = arg1;
    instruction.call.arguments[1] = arg2;
    instruction.call.arguments[2] = arg2;
    append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_mov_reg_type(ir_instruction_t* value, ir_register_type_t new_type, ir_value_type_t size, u8 new_reg) {
    if (IR_IS_GPR(new_reg) && new_type != REGISTER_TYPE_GPR) {
        logfatal("Trying to move value to a GPR, but register given was not a GPR!");
    } else if (IR_IS_FGR(new_reg) && new_type != REGISTER_TYPE_FGR_32 && new_type != REGISTER_TYPE_FGR_64) {
        logfatal("Trying to move value to an FGR, but register given was not a FGR!");
    }
    ir_instruction_t instruction;
    instruction.type = IR_MOV_REG_TYPE;
    instruction.mov_reg_type.value = value;
    instruction.mov_reg_type.new_type = new_type;
    instruction.mov_reg_type.size = size;
    return append_ir_instruction(instruction, -1, new_reg);
}

ir_instruction_t* ir_emit_float_convert(int index, ir_instruction_t* value, ir_float_value_type_t from_type, ir_float_value_type_t to_type, u8 guest_reg, ir_float_convert_mode_t convert_mode) {
    if (from_type == FLOAT_VALUE_TYPE_INVALID) {
        logfatal("Cannot convert from FLOAT_VALUE_TYPE_INVALID");
    }

    if (to_type == FLOAT_VALUE_TYPE_INVALID) {
        logfatal("Cannot convert to FLOAT_VALUE_TYPE_INVALID");
    }

    if (!IR_IS_FGR(guest_reg)) {
        logfatal("float convert must target an FGR");
    }

    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_CONVERT;
    instruction.float_convert.value = value;
    instruction.float_convert.from_type = from_type;
    instruction.float_convert.to_type = to_type;
    instruction.float_convert.mode = convert_mode;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_float_mult(int index, ir_instruction_t* multiplicand1, ir_instruction_t* multiplicand2, ir_float_value_type_t mult_type, u8 guest_reg) {
    if (!IR_IS_FGR(guest_reg)) {
        logfatal("float multiply must target an FGR");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_MULTIPLY;
    instruction.float_bin_op.operand1 = multiplicand1;
    instruction.float_bin_op.operand2 = multiplicand2;
    instruction.float_bin_op.format = mult_type;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_float_div(int index, ir_instruction_t* dividend, ir_instruction_t* divisor, ir_float_value_type_t divide_type, u8 guest_reg) {
    if (!IR_IS_FGR(guest_reg)) {
        logfatal("float divide must target an FGR");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_DIVIDE;
    instruction.float_bin_op.operand1 = dividend;
    instruction.float_bin_op.operand2 = divisor;
    instruction.float_bin_op.format = divide_type;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_float_add(int index, ir_instruction_t* operand1, ir_instruction_t* operand2, ir_float_value_type_t add_type, u8 guest_reg) {
    if (!IR_IS_FGR(guest_reg)) {
        logfatal("float add must target an FGR");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_ADD;
    instruction.float_bin_op.operand1 = operand1;
    instruction.float_bin_op.operand2 = operand2;
    instruction.float_bin_op.format = add_type;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_float_sub(int index, ir_instruction_t* operand1, ir_instruction_t* operand2, ir_float_value_type_t add_type, u8 guest_reg) {
    if (!IR_IS_FGR(guest_reg)) {
        logfatal("float add must target an FGR");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_SUB;
    instruction.float_bin_op.operand1 = operand1;
    instruction.float_bin_op.operand2 = operand2;
    instruction.float_bin_op.format = add_type;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_float_sqrt(int index, ir_instruction_t* operand, ir_float_value_type_t sqrt_type, u8 guest_reg) {
    if (!IR_IS_FGR(guest_reg)) {
        logfatal("float sqrt must target an FGR");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_SQRT;
    instruction.float_unary_op.operand = operand;
    instruction.float_unary_op.format = sqrt_type;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_float_abs(int index, ir_instruction_t* operand, ir_float_value_type_t abs_type, u8 guest_reg) {
    if (!IR_IS_FGR(guest_reg)) {
        logfatal("float abs must target an FGR");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_ABS;
    instruction.float_unary_op.operand = operand;
    instruction.float_unary_op.format = abs_type;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_float_neg(int index, ir_instruction_t* operand, ir_float_value_type_t neg_type, u8 guest_reg) {
    if (!IR_IS_FGR(guest_reg)) {
        logfatal("float neg must target an FGR");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_NEG;
    instruction.float_unary_op.operand = operand;
    instruction.float_unary_op.format = neg_type;
    return append_ir_instruction(instruction, index, guest_reg);
}

ir_instruction_t* ir_emit_float_check_condition(ir_float_condition_t cond, ir_instruction_t* operand1, ir_instruction_t* operand2, ir_float_value_type_t operand_type) {
    ir_instruction_t instruction;
    instruction.type = IR_FLOAT_CHECK_CONDITION;
    instruction.float_check_condition.operand1 = operand1;
    instruction.float_check_condition.operand2 = operand2;
    instruction.float_check_condition.condition = cond;
    instruction.float_check_condition.format = operand_type;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_rsp_lwc2(ir_instruction_t* addr, ir_instruction_t* old_value, rsp_lwc2_instruction_t type, u8 element, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_RSP_LWC2;
    instruction.rsp_lwc2.type = type;
    instruction.rsp_lwc2.addr = addr;
    instruction.rsp_lwc2.old_value = old_value;
    instruction.rsp_lwc2.element = element;
    return append_ir_instruction(instruction, -1, guest_reg);
}

ir_instruction_t* ir_emit_rsp_swc2(ir_instruction_t* addr, ir_instruction_t* value, rsp_swc2_instruction_t type, u8 element) {
    ir_instruction_t instruction;
    instruction.type = IR_RSP_SWC2;
    instruction.rsp_swc2.type = type;
    instruction.rsp_swc2.addr = addr;
    instruction.rsp_swc2.value = value;
    instruction.rsp_swc2.element = element;
    return append_ir_instruction(instruction, -1, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_ir_vpr_insert(ir_instruction_t* old_value, ir_instruction_t* value_to_insert, ir_value_type_t value_type, u8 byte_offset, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_VPR_INSERT;
    instruction.vpr_insert.old_value = old_value;
    instruction.vpr_insert.value_to_insert = value_to_insert;
    instruction.vpr_insert.value_type = value_type;
    instruction.vpr_insert.byte_offset = byte_offset;
    return append_ir_instruction(instruction, -1, guest_reg);
}