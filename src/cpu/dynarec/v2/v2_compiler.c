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

#define N64_LOG_COMPILATIONS

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
static void* link_and_encode(dasm_State** d) {
    size_t code_size;
    dasm_link(d, &code_size);
    void* buf = dynarec_bumpalloc(code_size);
    dasm_encode(d, buf);

#ifdef N64_LOG_COMPILATIONS
    printf("Generated %ld bytes of code\n", code_size);
    char* disassembly = malloc(4096);
    disassemble_x86_64((uintptr_t)buf, buf, code_size, disassembly, 4096);
    printf("%s", disassembly);
/*
    FILE* f = fopen("compiled.bin", "wb");
    fwrite(buf, 1, code_size, f);
    fclose(f);
    */
#endif

    return buf;
}

void compile_ir_or(dasm_State** Dst, ir_instruction_t* instr) {
    logfatal("Emitting IR_OR");
}

void compile_ir_and(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (is_constant(instr->bin_op.operand1)) {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand2->allocated_host_register);
        host_emit_and_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand1->set_constant);
    } else if (is_constant(instr->bin_op.operand2)) {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand1->allocated_host_register);
        host_emit_and_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand2->set_constant);
    } else {
        logfatal("Emitting IR_AND with two variable regs");
    }
}

void compile_ir_add(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (is_constant(instr->bin_op.operand1)) {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand2->allocated_host_register);
        host_emit_add_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand1->set_constant);
    } else if (is_constant(instr->bin_op.operand2)) {
        host_emit_mov_reg_reg(Dst, instr->allocated_host_register, instr->bin_op.operand1->allocated_host_register);
        host_emit_add_reg_imm(Dst, instr->allocated_host_register, instr->bin_op.operand2->set_constant);
    } else {
        logfatal("Emitting IR_AND with two variable regs");
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
        host_emit_mov_reg_reg(Dst, get_func_arg_registers()[arg_index], val->allocated_host_register);
    }
}

void compile_ir_store(dasm_State** Dst, ir_instruction_t* instr) {
    // If the address is known and is memory, it can be compiled as a direct store to memory
    if (is_constant(instr->store.address) && is_memory(const_to_u64(instr->store.address))) {
        logfatal("Emitting IR_STORE directly to memory");
    } else {
        switch (instr->store.type) {
            case VALUE_TYPE_S16:
            case VALUE_TYPE_U16:
                logfatal("Store 16 bit");
                break;
            case VALUE_TYPE_S32:
            case VALUE_TYPE_U32:
                val_to_func_arg(Dst, instr->store.address, 0);
                val_to_func_arg(Dst, instr->store.value, 1);
                host_emit_call(Dst, (uintptr_t)n64_write_physical_word);
                break;
            case VALUE_TYPE_64:
                logfatal("Store 64 bit");
                break;
        }
    }
}

void compile_ir_load(dasm_State** Dst, ir_instruction_t* instr) {
    // If the address is known and is memory, it can be compiled as a direct load from memory
    if (is_constant(instr->load.address) && is_memory(const_to_u64(instr->load.address))) {
        logfatal("Emitting IR_LOAD directly from memory");
    } else {
        switch (instr->load.type) {
            case VALUE_TYPE_S16:
            case VALUE_TYPE_U16:
                logfatal("Load 16 bit");
                break;
            case VALUE_TYPE_S32:
            case VALUE_TYPE_U32:
                val_to_func_arg(Dst, instr->load.address, 0);
                host_emit_call(Dst, (uintptr_t)n64_read_physical_word);
                host_emit_mov_reg_reg(Dst, instr->allocated_host_register, get_return_value_reg());
                break;
            case VALUE_TYPE_64:
                logfatal("Load 64 bit");
                break;
        }
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
        logfatal("Check condition with two variable regs");
    }
}

void compile_ir_set_cond_block_exit_pc(dasm_State** Dst, ir_instruction_t* instr) {
    ir_context.block_end_pc_set = true;
    if (is_constant(instr->set_cond_exit_pc.condition)) {
        logfatal("Set exit PC with const condition");
    } else {
        host_emit_cmov_pc_binary(Dst, instr->set_cond_exit_pc.condition->allocated_host_register, instr->set_cond_exit_pc.pc_if_true, instr->set_cond_exit_pc.pc_if_false);
    }
}

void compile_ir_set_block_exit_pc(dasm_State** Dst, ir_instruction_t* instr) {
    ir_context.block_end_pc_set = true;
    host_emit_mov_pc(Dst, instr->set_exit_pc.address);
}


void compile_ir_tlb_lookup(dasm_State** Dst, ir_instruction_t* instr) {
    val_to_func_arg(Dst, instr->tlb_lookup.virtual_address, 0);

    ir_set_constant_t bus_access;
    bus_access.type = VALUE_TYPE_U16;
    bus_access.value_u16 = instr->tlb_lookup.bus_access;
    host_emit_mov_reg_imm(Dst, get_func_arg_registers()[1], bus_access);

    host_emit_call(Dst, (uintptr_t)resolve_virtual_address_or_die);
    host_emit_mov_reg_reg(Dst, instr->allocated_host_register, get_return_value_reg());
}

void compile_ir_flush_guest_reg(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->flush_guest_reg.value)) {
        host_emit_mov_mem_imm(Dst, (uintptr_t)&N64CPU.gpr[instr->flush_guest_reg.guest_reg], instr->flush_guest_reg.value->set_constant);
    } else {
        host_emit_mov_mem_reg(Dst, (uintptr_t)&N64CPU.gpr[instr->flush_guest_reg.guest_reg], instr->flush_guest_reg.value->allocated_host_register);
    }
}

void compile_ir_load_guest_reg(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_mov_reg_mem(Dst, instr->allocated_host_register, (uintptr_t)&N64CPU.gpr[instr->load_guest_reg.guest_reg]);
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
                logfatal("Emitting IR_MASK_AND_CAST");
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
        }
        instr = instr->next;
    }
    DONE_COMPILING:
    // TODO: emit end block PC
    if (!ir_context.block_end_pc_set) {
        logfatal("TODO: emit end of block PC");
    }
    v2_end_block(Dst, temp_code_len);
    void* compiled = link_and_encode(&d);
    dasm_free(&d);

    block->run = compiled;

}

void v2_compile_new_block(
        n64_dynarec_block_t* block,
        bool* code_mask,
        u64 virtual_address,
        u32 physical_address) {

    fill_temp_code(virtual_address, physical_address, code_mask);
    ir_context_reset();
    printf("Translating to IR:\n");
    for (int i = 0; i < temp_code_len; i++) {
        u64 instr_virtual_address = virtual_address + (i << 2);
        u32 instr_physical_address = physical_address + (i << 2);
        emit_instruction_ir(temp_code[i].instr, instr_virtual_address, instr_physical_address);
    }
    // Flush all guest regs in use at the end
    for (int i = 1; i < 32; i++) {
        ir_instruction_t* val = ir_context.guest_gpr_to_value[i];
        if (val) {
            // If the guest reg was just loaded and never modified, don't need to flush it
            if (val->type != IR_LOAD_GUEST_REG) {
                ir_emit_flush_guest_reg(ir_context.guest_gpr_to_value[i], i);
            }
        }
    }
    print_ir_block();
    printf("Optimizing IR: constant propagation\n");
    ir_optimize_constant_propagation();
    print_ir_block();
    printf("Optimizing IR: eliminating dead code\n");
    ir_optimize_eliminate_dead_code();
    ir_optimize_shrink_constants();
    print_ir_block();
    ir_allocate_registers();
    v2_emit_block(block);
}

void v2_compiler_init() {
    uintptr_t n64_cpu_addr = (uintptr_t)&N64CPU;
    if (n64_cpu_addr > 0x7fffffff) {
        logwarn("N64 CPU was not statically allocated in the low 2GiB of the address space, the recompiler will not be able to use absolute addressing. It was allocated at: %016lX", n64_cpu_addr);
    }
}
