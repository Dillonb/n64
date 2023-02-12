#include "v2_compiler.h"

#include <mem/n64bus.h>
#include <disassemble.h>
#include <dynarec/dynarec_memory_management.h>
#include "v2_emitter.h"

#include "instruction_category.h"
#include "ir_emitter.h"
#include "ir_context.h"
#include "ir_optimizer.h"
#include "target_platform.h"

//#define N64_LOG_COMPILATIONS

typedef struct source_instruction {
    mips_instruction_t instr;
    dynarec_instruction_category_t category;
} source_instruction_t;

// Extra slot for the edge case where the branch delay slot is in the next page
#define TEMP_CODE_SIZE (BLOCKCACHE_INNER_SIZE + 1)
int temp_code_len = 0;
source_instruction_t temp_code[TEMP_CODE_SIZE];

// Determine what instructions should be compiled into the block and load them into temp_code
void fill_temp_code(u64 virtual_address, u32 physical_address, bool* code_mask) {
    bool should_continue_block = true;
    int instructions_left_in_block = -1;

    temp_code_len = 0;
#ifdef N64_LOG_COMPILATIONS
    printf("Starting a new block:\n");
#endif
    for (int i = 0; i < TEMP_CODE_SIZE; i++) {
        u32 instr_address = physical_address + (i << 2);
        u32 next_instr_address = instr_address + 4;
        u64 instr_virtual_address = virtual_address + (i << 2);

        bool page_boundary_ends_block = IS_PAGE_BOUNDARY(next_instr_address);

        dynarec_instruction_category_t prev_instr_category = NORMAL;
        if (i > 0) {
            prev_instr_category = temp_code[i - 1].category;
        }

        code_mask[BLOCKCACHE_INNER_INDEX(instr_address)] = true;

        temp_code[i].instr.raw = n64_read_physical_word(instr_address);
        temp_code[i].category = instr_category(temp_code[i].instr);
        temp_code_len++;
        instructions_left_in_block--;

        bool instr_ends_block;
        switch (temp_code[i].category) {
            // Possible to end block
            case CACHE:
            case STORE:
            case TLB_WRITE:
            case NORMAL:
                instr_ends_block = instructions_left_in_block == 0;
                break;

            // end block, but need to emit one more instruction
            case BRANCH:
            case BRANCH_LIKELY:
                if (is_branch(prev_instr_category)) {
                    logfatal("Branch in a branch delay slot");
                }
                instr_ends_block = false;
                instructions_left_in_block = 1; // emit delay slot
                break;

            // End block immediately
            case BLOCK_ENDER:
                instr_ends_block = true;
                break;
        }

        // If we still need to emit the delay slot, emit it, even if it's in the next block.
        // NOTE: this could cause problems. TODO: handle it somehow
        if (instructions_left_in_block == 1) {
            if (page_boundary_ends_block) {
                logwarn("including an instruction in a delay slot from the next page in a block associated with the previous page!");
                page_boundary_ends_block = false;
            }
        }

#ifdef N64_LOG_COMPILATIONS
        static char buf[50];
        disassemble(instr_virtual_address, temp_code[i].instr.raw, buf, 50);
        printf("%d [%08X]=%08X %s\n", i, (u32)instr_virtual_address, temp_code[i].instr.raw, buf);
#endif


        if (instr_ends_block || page_boundary_ends_block) {
#ifdef N64_LOG_COMPILATIONS
            printf("Ending block after %d instructions\n", temp_code_len);
#endif
            break;
        }
    }

    // If we filled up the buffer, make sure the last instruction is not a branch
    if (temp_code_len == TEMP_CODE_SIZE && is_branch(temp_code[TEMP_CODE_SIZE - 1].category)) {
        logwarn("Filled temp_code buffer, but the last instruction was a branch. Stripping it out.");
        temp_code_len--;
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
static void* link_and_encode(dasm_State** d, size_t* code_size_result) {
    size_t code_size;
    dasm_link(d, &code_size);
    void* buf = dynarec_bumpalloc(code_size);
    dasm_encode(d, buf);

#ifdef N64_LOG_COMPILATIONS
    printf("Generated %ld bytes of code\n", code_size);
/*
    FILE* f = fopen("compiled.bin", "wb");
    fwrite(buf, 1, code_size, f);
    fclose(f);
    */
#endif

    if (code_size_result) {
        *code_size_result = code_size;
    }
    return buf;
}

void compile_ir_or(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (is_constant(instr->bin_op.operand1)) {
        u64 operand1 = const_to_u64(instr->bin_op.operand1);
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand2->allocated_host_register, VALUE_TYPE_U64);
        if (operand1 != 0) { // TODO: catch this earlier in constant propagation
            host_emit_or_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand1->set_constant);
        }
    } else if (is_constant(instr->bin_op.operand2)) {
        u64 operand2 = const_to_u64(instr->bin_op.operand2);
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand1->allocated_host_register, VALUE_TYPE_U64);
        if (operand2 != 0) { // TODO: catch this earlier in constant propagation
            host_emit_or_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand2->set_constant);
        }
    } else {
        logfatal("Emitting IR_AND with two variable regs");
    }
}

void compile_ir_and(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (is_constant(instr->bin_op.operand1)) {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand2->allocated_host_register, VALUE_TYPE_U64);
        host_emit_and_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand1->set_constant);
    } else if (is_constant(instr->bin_op.operand2)) {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand1->allocated_host_register, VALUE_TYPE_U64);
        host_emit_and_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand2->set_constant);
    } else {
        if (instr->allocated_host_register == instr->bin_op.operand1->allocated_host_register) {
            // operand1 matches, do `and operand1, operand2`
            logfatal("operand1 matches, do `and operand1, operand2`");
        } else if (instr->allocated_host_register == instr->bin_op.operand2->allocated_host_register) {
            // operand2 matches, do `and operand2, operand1`
            logfatal("operand2 matches, do `and operand2, operand1`");
        } else {
            host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand1->allocated_host_register, VALUE_TYPE_U64);
            host_emit_and_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand2->allocated_host_register);
        }
    }
}

void compile_ir_not(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->unary_op.operand)) {
        logfatal("Should have been caught by constant propagation");
    } else {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->unary_op.operand->allocated_host_register, VALUE_TYPE_U64);
        host_emit_not(Dst, instr->allocated_host_register);
    }
}

void compile_ir_add(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (is_constant(instr->bin_op.operand1)) {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand2->allocated_host_register, VALUE_TYPE_U64);
        host_emit_add_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand1->set_constant);
    } else if (is_constant(instr->bin_op.operand2)) {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand1->allocated_host_register, VALUE_TYPE_U64);
        host_emit_add_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand2->set_constant);
    } else {
        if (instr->allocated_host_register == instr->bin_op.operand1->allocated_host_register) {
            logfatal("operand1 matches");
        } else if (instr->allocated_host_register == instr->bin_op.operand2->allocated_host_register) {
            logfatal("operand2 matches");
        } else {
            host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand1->allocated_host_register, VALUE_TYPE_U64);
            host_emit_add_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand2->allocated_host_register);
        }
    }
}

bool is_memory(u64 address) {
    return false; // TODO
}

void val_to_func_arg(dasm_State** Dst, ir_instruction_t* val, int arg_index) {
    if (arg_index >= get_num_func_arg_registers()) {
        logfatal("Too many args (%d) passed to fit into registers", arg_index + 1);
    }
    if (is_constant(val) && is_valid_immediate(val->set_constant.type)) {
        host_emit_mov_reg_imm(Dst, get_func_arg_registers()[arg_index], val->set_constant);
    } else {
        host_emit_mov_reg_reg(Dst, get_func_arg_registers()[arg_index], val->allocated_host_register, VALUE_TYPE_U64);
    }
}

void compile_ir_store(dasm_State** Dst, ir_instruction_t* instr) {
    // If the address is known and is memory, it can be compiled as a direct store to memory
    if (is_constant(instr->store.address) && is_memory(const_to_u64(instr->store.address))) {
        logfatal("Emitting IR_STORE directly to memory");
    } else {
        switch (instr->store.type) {
            case VALUE_TYPE_S8:
            case VALUE_TYPE_U8:
                val_to_func_arg(Dst, instr->store.address, 0);
                val_to_func_arg(Dst, instr->store.value, 1);
                host_emit_call(Dst, (uintptr_t)n64_write_physical_byte);
                break;
            case VALUE_TYPE_S16:
            case VALUE_TYPE_U16:
                val_to_func_arg(Dst, instr->store.address, 0);
                val_to_func_arg(Dst, instr->store.value, 1);
                host_emit_call(Dst, (uintptr_t)n64_write_physical_half);
                break;
            case VALUE_TYPE_S32:
            case VALUE_TYPE_U32:
                val_to_func_arg(Dst, instr->store.address, 0);
                val_to_func_arg(Dst, instr->store.value, 1);
                host_emit_call(Dst, (uintptr_t)n64_write_physical_word);
                break;
            case VALUE_TYPE_U64:
            case VALUE_TYPE_S64:
                val_to_func_arg(Dst, instr->store.address, 0);
                val_to_func_arg(Dst, instr->store.value, 1);
                host_emit_call(Dst, (uintptr_t)n64_write_physical_dword);
                break;
        }
    }
}

void compile_ir_load(dasm_State** Dst, ir_instruction_t* instr) {
    // If the address is known and is memory, it can be compiled as a direct load from memory
    if (is_constant(instr->load.address) && is_memory(const_to_u64(instr->load.address))) {
        logfatal("Emitting IR_LOAD directly from memory");
    } else {
        uintptr_t fp;
        switch (instr->load.type) {
            case VALUE_TYPE_S8:
            case VALUE_TYPE_U8:
                fp = (uintptr_t)n64_read_physical_byte;
                break;
            case VALUE_TYPE_S16:
            case VALUE_TYPE_U16:
                fp = (uintptr_t)n64_read_physical_half;
                break;
            case VALUE_TYPE_S32:
            case VALUE_TYPE_U32:
                fp = (uintptr_t)n64_read_physical_word;
                break;
            case VALUE_TYPE_U64:
            case VALUE_TYPE_S64:
                fp = (uintptr_t)n64_read_physical_dword;
                break;
        }

        val_to_func_arg(Dst, instr->load.address, 0);
        host_emit_call(Dst, fp);
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, get_return_value_reg(), instr->load.type);
    }
}

void compile_ir_mask_and_cast(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->mask_and_cast.operand)) {
        logfatal("Should have been caught by constant propagation");
    } else {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->mask_and_cast.operand->allocated_host_register, instr->mask_and_cast.type);
    }
}

void compile_ir_check_condition(dasm_State** Dst, ir_instruction_t* instr) {
    bool op1_const = is_constant(instr->check_condition.operand1);
    bool op2_const = is_constant(instr->check_condition.operand2);

    if (op1_const && op2_const) {
        logfatal("Should have been caught by constant propagation");
    } else if (op1_const) {
        host_emit_cmp_reg_imm(Dst, instr->allocated_host_register, instr->check_condition.condition, instr->check_condition.operand2->allocated_host_register, instr->check_condition.operand1->set_constant, ARGS_REVERSED);
    } else if (op2_const) {
        host_emit_cmp_reg_imm(Dst, instr->allocated_host_register, instr->check_condition.condition, instr->check_condition.operand1->allocated_host_register, instr->check_condition.operand2->set_constant, ARGS_NORMAL_ORDER);
    } else {
        host_emit_cmp_reg_reg(Dst, instr->allocated_host_register, instr->check_condition.condition, instr->check_condition.operand1->allocated_host_register, instr->check_condition.operand2->allocated_host_register, ARGS_NORMAL_ORDER);
    }
}

void compile_ir_set_cond_block_exit_pc(dasm_State** Dst, ir_instruction_t* instr) {
    ir_context.block_end_pc_compiled = true;
    if (is_constant(instr->set_cond_exit_pc.condition)) {
        logfatal("Set exit PC with const condition");
    } else {
        host_emit_cmov_pc_binary(Dst, instr->set_cond_exit_pc.condition->allocated_host_register, instr->set_cond_exit_pc.pc_if_true, instr->set_cond_exit_pc.pc_if_false);
    }
}

void compile_ir_set_block_exit_pc(dasm_State** Dst, ir_instruction_t* instr) {
    ir_context.block_end_pc_compiled = true;
    host_emit_mov_pc(Dst, instr->unary_op.operand);
}


void compile_ir_tlb_lookup(dasm_State** Dst, ir_instruction_t* instr) {
    val_to_func_arg(Dst, instr->tlb_lookup.virtual_address, 0);

    ir_set_constant_t bus_access;
    bus_access.type = VALUE_TYPE_U16;
    bus_access.value_u16 = instr->tlb_lookup.bus_access;
    host_emit_mov_reg_imm(Dst, get_func_arg_registers()[1], bus_access);

    host_emit_call(Dst, (uintptr_t)resolve_virtual_address_or_die);
    host_emit_mov_reg_reg(Dst, instr->allocated_host_register, get_return_value_reg(), VALUE_TYPE_U64);
}

void compile_ir_flush_guest_reg(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->flush_guest_reg.value)) {
        host_emit_mov_mem_imm(Dst, (uintptr_t)&N64CPU.gpr[instr->flush_guest_reg.guest_reg], instr->flush_guest_reg.value->set_constant, VALUE_TYPE_U64);
    } else {
        host_emit_mov_mem_reg(Dst, (uintptr_t)&N64CPU.gpr[instr->flush_guest_reg.guest_reg], instr->flush_guest_reg.value->allocated_host_register, VALUE_TYPE_U64);
    }
}

void compile_ir_shift(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->shift.operand)) {
        logfatal("Shift const operand");
    } else {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->shift.operand->allocated_host_register, VALUE_TYPE_U64);
        if (is_constant(instr->shift.amount)) {
            u64 shift_amount_64 = const_to_u64(instr->shift.amount);
            u8 shift_amount = shift_amount_64;
            if (shift_amount_64 != shift_amount) {
                logfatal("Const shift amount > 0xFF: %lu", shift_amount_64);
            }

            host_emit_shift_reg_imm(Dst, instr->allocated_host_register, instr->shift.type, shift_amount, instr->shift.direction);
        } else {
            logfatal("Shift variable operand and variable amount");
        }
    }
}

void compile_ir_load_guest_reg(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_mov_reg_mem(Dst, instr->allocated_host_register, (uintptr_t)&N64CPU.gpr[instr->load_guest_reg.guest_reg]);
}

void compile_ir_get_cp0(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_mov_reg_cp0(Dst, instr->allocated_host_register, instr->get_cp0.reg);
}

void compile_ir_set_cp0(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr)) {
        logfatal("set cp0 const");
    } else {
        host_emit_mov_cp0_reg(Dst, instr->get_cp0.reg, instr->allocated_host_register);
    }
}

void compile_ir_cond_block_exit(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr)) {
        logfatal("compile_ir_cond_block_exit with constant condition");
    } else {
        ir_instruction_flush_t* flush_iter = instr->cond_block_exit.regs_to_flush;
        host_emit_cond_ret(Dst, instr->cond_block_exit.condition->allocated_host_register, flush_iter, instr->cond_block_exit.block_length);
    }
}

void compile_ir_multiply(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->multiply.multiplicand1) && is_constant(instr->multiply.multiplicand2)) {
        logfatal("const mult");
    } else if (is_constant(instr->multiply.multiplicand1)) {
        host_emit_mult_reg_imm(Dst, instr->multiply.multiplicand2->allocated_host_register, instr->multiply.multiplicand1->set_constant, instr->multiply.multiplicand_type);
    } else if (is_constant(instr->multiply.multiplicand2)) {
        host_emit_mult_reg_imm(Dst, instr->multiply.multiplicand1->allocated_host_register, instr->multiply.multiplicand2->set_constant, instr->multiply.multiplicand_type);
    } else {
        logfatal("non const mult");
    }
}

void compile_ir_get_mult_result(dasm_State** Dst, ir_instruction_t* instr) {
    switch (instr->mult_result.result_bits) {
        case MULT_RESULT_HI:
            host_emit_mov_reg_mem(Dst, instr->allocated_host_register, (uintptr_t)&N64CPU.mult_hi);
            break;
        case MULT_RESULT_LO:
            host_emit_mov_reg_mem(Dst, instr->allocated_host_register, (uintptr_t)&N64CPU.mult_lo);
            break;
    }
}

void v2_emit_block(n64_dynarec_block_t* block) {
    static dasm_State* d;
    d = v2_block_header();
    dasm_State** Dst = &d;
    ir_instruction_t* instr = ir_context.ir_cache_head;
    while (instr) {
        switch (instr->type) {
            case IR_NOP: break;
            case IR_SET_CONSTANT:
                // Only load into a host register if it was determined by the allocator that we need to
                // Otherwise, it can be used as an immediate when needed
                if (instr->allocated_host_register >= 0) {
                    logfatal("Emit set constant");
                }
                break;
            case IR_OR:
                compile_ir_or(Dst, instr);
                break;
            case IR_AND:
                compile_ir_and(Dst, instr);
                break;
            case IR_NOT:
                compile_ir_not(Dst, instr);
                break;
            case IR_ADD:
                compile_ir_add(Dst, instr);
                break;
            case IR_STORE:
                compile_ir_store(Dst, instr);
                break;
            case IR_LOAD:
                compile_ir_load(Dst, instr);
                break;
            case IR_MASK_AND_CAST:
                compile_ir_mask_and_cast(Dst, instr);
                break;
            case IR_CHECK_CONDITION:
                compile_ir_check_condition(Dst, instr);
                break;
            case IR_SET_BLOCK_EXIT_PC:
                compile_ir_set_block_exit_pc(Dst, instr);
                break;
            case IR_SET_COND_BLOCK_EXIT_PC:
                compile_ir_set_cond_block_exit_pc(Dst, instr);
                break;
            case IR_TLB_LOOKUP:
                compile_ir_tlb_lookup(Dst, instr);
                break;
            case IR_LOAD_GUEST_REG:
                compile_ir_load_guest_reg(Dst, instr);
                break;
            case IR_FLUSH_GUEST_REG:
                compile_ir_flush_guest_reg(Dst, instr);
                break;
            case IR_SHIFT:
                compile_ir_shift(Dst, instr);
                break;
            case IR_GET_CP0:
                compile_ir_get_cp0(Dst, instr);
                break;
            case IR_SET_CP0:
                compile_ir_set_cp0(Dst, instr);
                break;
            case IR_COND_BLOCK_EXIT:
                compile_ir_cond_block_exit(Dst, instr);
                break;
            case IR_MULTIPLY:
                compile_ir_multiply(Dst, instr);
                break;
            case IR_GET_MULT_RESULT:
                compile_ir_get_mult_result(Dst, instr);
        }
        instr = instr->next;
    }
    DONE_COMPILING:
    // TODO: emit end block PC
    if (!ir_context.block_end_pc_compiled) {
        logfatal("TODO: emit end of block PC");
    }
    v2_end_block(Dst, temp_code_len);
    size_t code_size;
    void* compiled = link_and_encode(&d, &code_size);
    dasm_free(&d);

    block->run = compiled;
    block->guest_size = temp_code_len * 4;
    block->host_size = code_size;

}

void v2_compile_new_block(
        n64_dynarec_block_t* block,
        bool* code_mask,
        u64 virtual_address,
        u32 physical_address) {

    fill_temp_code(virtual_address, physical_address, code_mask);
    ir_context_reset();
#ifdef N64_LOG_COMPILATIONS
    printf("Translating to IR:\n");
#endif
    for (int i = 0; i < temp_code_len; i++) {
        u64 instr_virtual_address = virtual_address + (i << 2);
        u32 instr_physical_address = physical_address + (i << 2);
        emit_instruction_ir(temp_code[i].instr, i, instr_virtual_address, instr_physical_address);
    }
    ir_optimize_flush_guest_regs();
#ifdef N64_LOG_COMPILATIONS
    print_ir_block();
    printf("Optimizing:\n");
#endif
    ir_optimize_constant_propagation();
    ir_optimize_eliminate_dead_code();
    ir_optimize_shrink_constants();
    ir_allocate_registers();
#ifdef N64_LOG_COMPILATIONS
    print_ir_block();
    printf("Emitting to host code:\n");
#endif
    v2_emit_block(block);
}

void v2_compiler_init() {
    uintptr_t n64_cpu_addr = (uintptr_t)&N64CPU;
    if (n64_cpu_addr > 0x7fffffff) {
        logwarn("N64 CPU was not statically allocated in the low 2GiB of the address space, the recompiler will not be able to use absolute addressing. It was allocated at: %016lX", n64_cpu_addr);
    }
}
