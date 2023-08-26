#include "v2_compiler.h"

#include <log.h>
#include <mem/n64bus.h>
#include <disassemble.h>
#include <dynarec/dynarec_memory_management.h>
#include <r4300i.h>
#include <r4300i_register_access.h>
#include "v2_emitter.h"
#include <system/mprotect_utils.h>
#include <mips_instructions.h>
#include <system/scheduler.h>

#include "instruction_category.h"
#include "ir_emitter.h"
#include "ir_context.h"
#include "ir_optimizer.h"
#include "target_platform.h"
#include "register_allocator.h"

//#define N64_LOG_COMPILATIONS

static bool v2_idle_loop_detection_enabled = true;

#define DISPATCHER_CODE_SIZE 4096
static u8 run_block_codecache[DISPATCHER_CODE_SIZE] __attribute((aligned(4096)));

typedef struct source_instruction {
    mips_instruction_t instr;
    dynarec_instruction_category_t category;
} source_instruction_t;

// Extra slot for the edge case where the branch delay slot is in the next page
#define TEMP_CODE_SIZE (BLOCKCACHE_INNER_SIZE + 1)
#define MAX_BLOCK_LENGTH BLOCKCACHE_INNER_SIZE
int temp_code_len = 0;
source_instruction_t temp_code[TEMP_CODE_SIZE];
u64 temp_code_vaddr = 0;

#define LAST_INSTR_CATEGORY (temp_code[temp_code_len - 1].category)
#define LAST_INSTR_IS_BRANCH ((temp_code_len > 0) && ((LAST_INSTR_CATEGORY == BRANCH) || (LAST_INSTR_CATEGORY == BRANCH_LIKELY)))

u64 v2_get_last_compiled_block() {
    return temp_code_vaddr;
}

INLINE bool should_break(u32 address) {
#ifdef N64_DEBUG_MODE
    switch (address) {
        case 0xFFFFFFFF:
        //case 0x8F550:
        //case 0x8F5A0:
        //case 0x8F56C:
        //case 0x8F588:
        //case 0x3934:
            return true;
        default:
            return false;
    }
#else
    return false;
#endif
}

// Determine what instructions should be compiled into the block and load them into temp_code
void fill_temp_code(u64 virtual_address, u32 physical_address, bool* code_mask) {
    temp_code_vaddr = virtual_address;
    int instructions_left_in_block = -1;

    temp_code_len = 0;
#ifdef N64_LOG_COMPILATIONS
    printf("Starting a new block:\n");
#endif
    for (int i = 0; i < MAX_BLOCK_LENGTH || instructions_left_in_block > 0; i++) {
        u32 instr_address = physical_address + (i << 2);
        u32 next_instr_address = instr_address + 4;

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
                    logwarn("Branch in a branch delay slot!");
                    instr_ends_block = true; // Compiler will detect the block-ends-in-branch case and handle it appropriately
                    break;
                } else {
                    instr_ends_block = false;
                    instructions_left_in_block = 1; // emit delay slot
                }
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
                logwarn("including an instruction in a delay slot from the next page in a block associated with the previous page! Fall back to interpreter.");
            }
        }

#ifdef N64_LOG_COMPILATIONS
        static char buf[50];
        u64 instr_virtual_address = virtual_address + (i << 2);
        disassemble(instr_virtual_address, temp_code[i].instr.raw, buf, 50);
        printf("%d [%08X]=%08X %s\n", i, (u32)instr_virtual_address, temp_code[i].instr.raw, buf);
#endif


        if (instr_ends_block || page_boundary_ends_block) {
            break;
        }
    }

#ifdef N64_LOG_COMPILATIONS
    printf("Ending block after %d instructions\n", temp_code_len);
#endif

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

void compile_ir_set_constant(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_mov_reg_imm(Dst, instr->reg_alloc, instr->set_constant);
}

void compile_ir_or(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (instr_valid_immediate(instr->bin_op.operand1)) {
        u64 operand1 = const_to_u64(instr->bin_op.operand1);
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc, VALUE_TYPE_U64);
        if (operand1 != 0) { // TODO: catch this earlier in constant propagation
            host_emit_or_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand1->set_constant);
        }
    } else if (instr_valid_immediate(instr->bin_op.operand2)) {
        u64 operand2 = const_to_u64(instr->bin_op.operand2);
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        if (operand2 != 0) { // TODO: catch this earlier in constant propagation
            host_emit_or_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand2->set_constant);
        }
    } else {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        host_emit_or_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc);
    }
}

void compile_ir_xor(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (instr_valid_immediate(instr->bin_op.operand1)) {
        u64 operand1 = const_to_u64(instr->bin_op.operand1);
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc, VALUE_TYPE_U64);
        if (operand1 != 0) { // TODO: catch this earlier in constant propagation
            host_emit_xor_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand1->set_constant);
        }
    } else if (instr_valid_immediate(instr->bin_op.operand2)) {
        u64 operand2 = const_to_u64(instr->bin_op.operand2);
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        if (operand2 != 0) { // TODO: catch this earlier in constant propagation
            host_emit_xor_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand2->set_constant);
        }
    } else {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        host_emit_xor_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc);
    }
}

void compile_ir_and(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (instr_valid_immediate(instr->bin_op.operand1)) {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc, VALUE_TYPE_U64);
        host_emit_and_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand1->set_constant);
    } else if (instr_valid_immediate(instr->bin_op.operand2)) {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        host_emit_and_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand2->set_constant);
    } else {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        host_emit_and_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc);
    }
}

void compile_ir_not(dasm_State** Dst, ir_instruction_t* instr) {
    if (instr_valid_immediate(instr->unary_op.operand)) {
        logfatal("Should have been caught by constant propagation");
    } else {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->unary_op.operand->reg_alloc, VALUE_TYPE_U64);
        host_emit_bitwise_not(Dst, instr->reg_alloc);
    }
}

void compile_ir_add(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (instr_valid_immediate(instr->bin_op.operand1)) {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc, VALUE_TYPE_U64);
        host_emit_add_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand1->set_constant);
    } else if (instr_valid_immediate(instr->bin_op.operand2)) {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        host_emit_add_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand2->set_constant);
    } else {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        host_emit_add_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc);
    }
}

void compile_ir_sub(dasm_State** Dst, ir_instruction_t* instr) {
    if (binop_constant(instr)) {
        logfatal("Should have been caught by constant propagation");
    } else if (instr_valid_immediate(instr->bin_op.operand1)) {
        host_emit_mov_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand1->set_constant);
        host_emit_sub_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc);
    } else if (instr_valid_immediate(instr->bin_op.operand2)) {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        host_emit_sub_reg_imm(Dst, instr->reg_alloc, instr->bin_op.operand2->set_constant);
    } else {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand1->reg_alloc, VALUE_TYPE_U64);
        host_emit_sub_reg_reg(Dst, instr->reg_alloc, instr->bin_op.operand2->reg_alloc);
    }
}

bool is_memory(u64 address) {
    return false; // TODO
}

void val_to_func_arg(dasm_State** Dst, ir_instruction_t* val, int arg_index) {
    if (arg_index >= get_num_func_arg_registers()) {
        logfatal("Too many args (%d) passed to fit into registers", arg_index + 1);
    }
    if (instr_valid_immediate(val) && is_valid_immediate(val->set_constant.type)) {
        host_emit_mov_reg_imm(Dst, alloc_gpr(get_func_arg_registers()[arg_index]), val->set_constant);
    } else {
        host_emit_mov_reg_reg(Dst, alloc_gpr(get_func_arg_registers()[arg_index]), val->reg_alloc, VALUE_TYPE_U64);
    }
}

void compile_ir_store(dasm_State** Dst, ir_instruction_t* instr) {
    // If the address is known and is memory, it can be compiled as a direct store to memory
    if (instr_valid_immediate(instr->store.address) && is_memory(const_to_u64(instr->store.address))) {
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
    if (instr_valid_immediate(instr->load.address) && is_memory(const_to_u64(instr->load.address))) {
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
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, alloc_gpr(get_return_value_reg()), instr->load.type);
    }
}

void compile_ir_get_ptr(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_mov_reg_mem(Dst, instr->reg_alloc, instr->get_ptr.ptr, instr->get_ptr.type);
}

void compile_ir_set_ptr(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->set_ptr.value)) {
        host_emit_mov_mem_imm(Dst, instr->set_ptr.ptr, instr->set_ptr.value->set_constant, instr->set_ptr.type);
    } else {
        host_emit_mov_mem_reg(Dst, instr->set_ptr.ptr, instr->set_ptr.value->reg_alloc, instr->set_ptr.type);
    }
}

void compile_ir_mask_and_cast(dasm_State** Dst, ir_instruction_t* instr) {
    if (instr_valid_immediate(instr->mask_and_cast.operand)) {
        logfatal("Should have been caught by constant propagation");
    } else {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->mask_and_cast.operand->reg_alloc, instr->mask_and_cast.type);
    }
}

void compile_ir_check_condition(dasm_State** Dst, ir_instruction_t* instr) {
    bool op1_const = is_constant(instr->check_condition.operand1);
    bool op2_const = is_constant(instr->check_condition.operand2);
    bool op1_valid_immediate = instr_valid_immediate(instr->check_condition.operand1);
    bool op2_valid_immediate = instr_valid_immediate(instr->check_condition.operand2);

    if (op1_const && op2_const) {
        logfatal("Should have been caught by constant propagation");
    } else if (op1_valid_immediate) {
        host_emit_cmp_reg_imm(Dst, instr->reg_alloc, instr->check_condition.condition, instr->check_condition.operand2->reg_alloc, instr->check_condition.operand1->set_constant, ARGS_REVERSED);
    } else if (op2_valid_immediate) {
        host_emit_cmp_reg_imm(Dst, instr->reg_alloc, instr->check_condition.condition, instr->check_condition.operand1->reg_alloc, instr->check_condition.operand2->set_constant, ARGS_NORMAL_ORDER);
    } else {
        host_emit_cmp_reg_reg(Dst, instr->reg_alloc, instr->check_condition.condition, instr->check_condition.operand1->reg_alloc, instr->check_condition.operand2->reg_alloc, ARGS_NORMAL_ORDER);
    }
}

void compile_ir_set_cond_block_exit_pc(dasm_State** Dst, ir_instruction_t* instr) {
    ir_context.block_end_pc_compiled = true;
    if (is_constant(instr->set_cond_exit_pc.condition)) {
        logfatal("Set exit PC with const condition");
    } else {
        host_emit_cmov_pc_binary(Dst, instr->set_cond_exit_pc.condition->reg_alloc, instr->set_cond_exit_pc.pc_if_true, instr->set_cond_exit_pc.pc_if_false);
    }
}

void compile_ir_set_block_exit_pc(dasm_State** Dst, ir_instruction_t* instr) {
    ir_context.block_end_pc_compiled = true;
    host_emit_mov_pc(Dst, instr->unary_op.operand);
}

/**
 * @brief Resolves a virtual address in a way that's useful for the JIT.
 * 
 * @param virtual virtual address to resolve
 * @param except_pc pc of the mips instruction doing the resolve
 * @param bus_access is the resolve for a load or a store?
 * @return bits 0-31: resulting physical address, will be 0 if failure. bit 32: 1 if failed, 0 if succeeded
 */
u64 resolve_virtual_address_for_jit(u64 virtual, u64 except_pc, bus_access_t bus_access) {
    u32 physical = 0;

    if (resolve_virtual_address(virtual, bus_access, &physical)) {
        return physical;
    } else {
        on_tlb_exception(virtual);
        u32 code = get_tlb_exception_code(N64CP0.tlb_error, bus_access);
        r4300i_handle_exception(except_pc, code, 0);
        return 1ull << 32;
    }
}

void compile_ir_tlb_lookup(dasm_State** Dst, ir_instruction_t* instr) {
    if (instr->block_length <= 0) {
        logfatal("TLB lookup compiled with a block length of %d", instr->block_length);
    }
    bool prev_branch = instr->block_length > 1 && (temp_code[instr->block_length - 2].category == BRANCH || temp_code[instr->block_length - 2].category == BRANCH_LIKELY);
    static_assert(sizeof(N64CPU.prev_branch) == 1, "prev_branch should be one byte");

    ir_set_constant_t prev_branch_const = { .type = VALUE_TYPE_U8, .value_u8 = prev_branch };
    host_emit_mov_mem_imm(Dst, (uintptr_t)&N64CPU.prev_branch, prev_branch_const, VALUE_TYPE_U8);

    ir_register_allocation_t return_value_reg = alloc_gpr(get_return_value_reg());
    val_to_func_arg(Dst, instr->tlb_lookup.virtual_address, 0);

    // faulting pc for if an exception occurs
    ir_set_constant_t except_pc;
    except_pc.type = VALUE_TYPE_U64;
    except_pc.value_u64 = temp_code_vaddr + ((instr->block_length - 1) * 4u);
    host_emit_mov_reg_imm(Dst, alloc_gpr(get_func_arg_registers()[1]), except_pc);

    ir_set_constant_t bus_access;
    bus_access.type = VALUE_TYPE_U16;
    bus_access.value_u16 = instr->tlb_lookup.bus_access;
    host_emit_mov_reg_imm(Dst, alloc_gpr(get_func_arg_registers()[2]), bus_access);

    host_emit_call(Dst, (uintptr_t)resolve_virtual_address_for_jit);
    // Move the full value into the destination reg. Don't need to worry about the success bit, because if that bit is set, the return value is junk anyway.
    host_emit_mov_reg_reg(Dst, instr->reg_alloc, return_value_reg, VALUE_TYPE_U64);
    // Shift the success bit into bit 0
    host_emit_shift_reg_imm(Dst, return_value_reg, VALUE_TYPE_U64, 32, SHIFT_DIRECTION_RIGHT);
    host_emit_cond_ret(Dst, return_value_reg, &instr->flush_info, instr->block_length, COND_BLOCK_EXIT_TYPE_NONE, (cond_block_exit_info_t){});
}

void compile_ir_flush_guest_reg(dasm_State** Dst, ir_instruction_t* instr) {
    if (IR_IS_FGR(instr->flush_guest_reg.guest_reg)) {
        if (is_constant(instr->flush_guest_reg.value)) {
            logfatal("Flushing const FPU reg with fr=%d", N64CP0.status.fr);
        } else {
            ir_register_type_t reg_type = instr->flush_guest_reg.value->reg_alloc.type;
            if (reg_type == REGISTER_TYPE_FGR_64) {
                uintptr_t dest = (uintptr_t)get_fpu_register_ptr_dword_fr(instr->flush_guest_reg.guest_reg - IR_FGR_BASE);
                host_emit_mov_mem_reg(Dst, dest, instr->flush_guest_reg.value->reg_alloc, VALUE_TYPE_U64);
            } else if (reg_type == REGISTER_TYPE_FGR_32) {
                uintptr_t dest = (uintptr_t)get_fpu_register_ptr_dword_fr(instr->flush_guest_reg.guest_reg - IR_FGR_BASE);
                host_emit_mov_mem_reg(Dst, dest, instr->flush_guest_reg.value->reg_alloc, VALUE_TYPE_U64);
            } else {
                logfatal("Flushing non const FPU reg with unexpected reg_type %d", reg_type);
            }
        }
    } else {
        if (is_constant(instr->flush_guest_reg.value)) {
            host_emit_mov_mem_imm(Dst, (uintptr_t)&N64CPU.gpr[instr->flush_guest_reg.guest_reg], instr->flush_guest_reg.value->set_constant, VALUE_TYPE_U64);
        } else {
            host_emit_mov_mem_reg(Dst, (uintptr_t)&N64CPU.gpr[instr->flush_guest_reg.guest_reg], instr->flush_guest_reg.value->reg_alloc, VALUE_TYPE_U64);
        }
    }
}

void compile_ir_shift(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->shift.operand)) {
        host_emit_mov_reg_imm(Dst, instr->reg_alloc, instr->shift.operand->set_constant);
    } else {
        host_emit_mov_reg_reg(Dst, instr->reg_alloc, instr->shift.operand->reg_alloc, VALUE_TYPE_U64);
    }

    if (is_constant(instr->shift.amount)) {
        u64 shift_amount_64 = const_to_u64(instr->shift.amount);
        u8 shift_amount = shift_amount_64;
        if (shift_amount_64 != shift_amount) {
            logfatal("Const shift amount > 0xFF: %" PRIu64, shift_amount_64);
        }

        host_emit_shift_reg_imm(Dst, instr->reg_alloc, instr->shift.type, shift_amount, instr->shift.direction);
    } else {
        host_emit_shift_reg_reg(Dst, instr->reg_alloc, instr->shift.type, instr->shift.amount->reg_alloc, instr->shift.direction);
    }
}

void compile_ir_load_guest_reg(dasm_State** Dst, ir_instruction_t* instr) {
    if (instr->reg_alloc.type != instr->load_guest_reg.guest_reg_type) {
        logwarn("Wrong type of register allocated! Wanted %d but got %d\n", instr->load_guest_reg.guest_reg_type, instr->reg_alloc.type);
    }
    switch (instr->load_guest_reg.guest_reg_type) {
        case REGISTER_TYPE_NONE:
            logfatal("Loading reg of type NONE");
            break;
        case REGISTER_TYPE_GPR:
            unimplemented(!IR_IS_GPR(instr->load_guest_reg.guest_reg), "Loading a GPR, but register is not a GPR!");
            host_emit_mov_reg_mem(Dst, instr->reg_alloc, (uintptr_t)&N64CPU.gpr[instr->load_guest_reg.guest_reg], VALUE_TYPE_U64);
            break;
        case REGISTER_TYPE_FGR_32:
            unimplemented(!IR_IS_FGR(instr->load_guest_reg.guest_reg), "Loading an FGR_32, but register is not an FGR!");
            host_emit_mov_reg_mem(Dst, instr->reg_alloc, (uintptr_t)get_fpu_register_ptr_word_fr(instr->load_guest_reg.guest_reg - IR_FGR_BASE), VALUE_TYPE_U32);
            break;
        case REGISTER_TYPE_FGR_64:
            unimplemented(!IR_IS_FGR(instr->load_guest_reg.guest_reg), "Loading an FGR_64, but register is not an FGR!");
            host_emit_mov_reg_mem(Dst, instr->reg_alloc, (uintptr_t)get_fpu_register_ptr_dword_fr(instr->load_guest_reg.guest_reg - IR_FGR_BASE), VALUE_TYPE_U64);
            break;
    }
}

void compile_ir_cond_block_exit(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->cond_block_exit.condition)) {
        bool cond = const_to_u64(instr->cond_block_exit.condition) != 0;
        if (cond) {
            switch (instr->cond_block_exit.type) {
                case COND_BLOCK_EXIT_TYPE_NONE:
                    break;
                case COND_BLOCK_EXIT_TYPE_EXCEPTION:
                    host_emit_exception_to_args(Dst, instr->cond_block_exit.info.exception);
                    host_emit_call(Dst, (uintptr_t)r4300i_handle_exception);
                    break;
                case COND_BLOCK_EXIT_TYPE_ADDRESS:
                    host_emit_mov_pc(Dst, instr->cond_block_exit.info.exit_pc);
                    break;
            }
            host_emit_ret(Dst, &instr->flush_info, instr->block_length);
        }
    } else {
        host_emit_cond_ret(Dst, instr->cond_block_exit.condition->reg_alloc, &instr->flush_info, instr->block_length, instr->cond_block_exit.type, instr->cond_block_exit.info);
    }
}

void evaluate_const_multiply(dasm_State** Dst, ir_set_constant_t operand1, ir_set_constant_t operand2, ir_value_type_t type) {
    ir_set_constant_t result_lo;
    ir_set_constant_t result_hi;

    result_lo.type = VALUE_TYPE_S64;
    result_hi.type = VALUE_TYPE_S64;

    switch (type) {
        case VALUE_TYPE_U8:
            logfatal("const VALUE_TYPE_U8 multiply");
            break;
        case VALUE_TYPE_S8:
            logfatal("const VALUE_TYPE_S8 multiply");
            break;
        case VALUE_TYPE_S16:
            logfatal("const VALUE_TYPE_S16 multiply");
            break;
        case VALUE_TYPE_U16:
            logfatal("const VALUE_TYPE_U16 multiply");
            break;
        case VALUE_TYPE_S32: {
            s64 multiplicand_1 = set_const_to_s32(operand1);
            s64 multiplicand_2 = set_const_to_s32(operand2);

            s64 result = multiplicand_1 * multiplicand_2;

            s32 result_lower = result & 0xFFFFFFFF;
            s32 result_upper = (result >> 32) & 0xFFFFFFFF;

            result_lo.value_s64 = (s64)result_lower;
            result_hi.value_s64 = (s64)result_upper;
            break;
        }
        case VALUE_TYPE_U32: {
            u64 multiplicand_1 = set_const_to_u32(operand1);
            u64 multiplicand_2 = set_const_to_u32(operand2);

            u64 result = multiplicand_1 * multiplicand_2;

            s32 result_lower = result & 0xFFFFFFFF;
            s32 result_upper = (result >> 32) & 0xFFFFFFFF;

            result_lo.value_s64 = (s64)result_lower;
            result_hi.value_s64 = (s64)result_upper;
            break;
        }
        case VALUE_TYPE_U64:
            result_lo.value_s64 = multu_64_to_128(set_const_to_u64(operand1), set_const_to_u64(operand2), (u64*)&result_hi.value_s64);
            break;
        case VALUE_TYPE_S64: {
            result_lo.value_s64 = mult_64_to_128(set_const_to_s64(operand1), set_const_to_s64(operand2), (u64*)&result_hi.value_s64);
            break;
        }
    }

    host_emit_mov_mem_imm(Dst, (uintptr_t)&N64CPU.mult_lo, result_lo, VALUE_TYPE_S64);
    host_emit_mov_mem_imm(Dst, (uintptr_t)&N64CPU.mult_hi, result_hi, VALUE_TYPE_S64);
}

void evaluate_const_divide(dasm_State** Dst, ir_set_constant_t operand_dividend, ir_set_constant_t operand_divisor, ir_value_type_t type) {
    ir_set_constant_t result_quotient;
    ir_set_constant_t result_remainder;

    result_quotient.type = VALUE_TYPE_S64;
    result_remainder.type = VALUE_TYPE_S64;

    switch (type) {
        case VALUE_TYPE_U8:
            logfatal("const VALUE_TYPE_U8 divide");
            break;
        case VALUE_TYPE_S8:
            logfatal("const VALUE_TYPE_S8 divide");
            break;
        case VALUE_TYPE_S16:
            logfatal("const VALUE_TYPE_S16 divide");
            break;
        case VALUE_TYPE_U16:
            logfatal("const VALUE_TYPE_U16 divide");
            break;
        case VALUE_TYPE_S32: {
            s64 dividend = set_const_to_s32(operand_dividend);
            s64 divisor = set_const_to_s32(operand_divisor);

            if (divisor == 0) {
                logwarn("Divide by zero");
                result_remainder.value_s64 = dividend;
                if (dividend >= 0) {
                    result_quotient.value_s64 = -1;
                } else {
                    result_quotient.value_s64 = 1;
                }
            } else {
                s32 quotient = dividend / divisor;
                s32 remainder = dividend % divisor;

                result_quotient.value_s64 = quotient;
                result_remainder.value_s64 = remainder;
            }
            break;
        }
        case VALUE_TYPE_U32: {
            u32 dividend = set_const_to_u32(operand_dividend);
            u32 divisor  = set_const_to_u32(operand_divisor);

            if (divisor == 0) {
                result_quotient.value_s64 = 0xFFFFFFFFFFFFFFFF;
                result_remainder.value_s64 = (s32)dividend;
            } else {
                s32 quotient  = dividend / divisor;
                s32 remainder = dividend % divisor;

                result_quotient.value_s64 = quotient;
                result_remainder.value_s64 = remainder;
            }
            break;
        }
        case VALUE_TYPE_U64: {
            u64 dividend = set_const_to_u64(operand_dividend);
            u64 divisor  = set_const_to_u64(operand_divisor);

            if (divisor == 0) {
                result_quotient.value_s64 = 0xFFFFFFFFFFFFFFFF;
                result_remainder.value_s64 = dividend;
            } else {
                u64 quotient  = dividend / divisor;
                u64 remainder = dividend % divisor;

                result_quotient.value_s64 = quotient;
                result_remainder.value_s64 = remainder;
            }
            break;
        }
        case VALUE_TYPE_S64: {
            s64 dividend = set_const_to_s64(operand_dividend);
            s64 divisor  = set_const_to_s64(operand_divisor);

            if (unlikely(divisor == 0)) {
                logwarn("Divide by zero");
                result_remainder.value_s64 = dividend;
                if (dividend >= 0) {
                    result_quotient.value_s64 = (s64)-1;
                } else {
                    result_quotient.value_s64 = (s64)1;
                }
            } else if (unlikely(divisor == -1 && dividend == INT64_MIN)) {
                result_quotient.value_s64 = dividend;
                result_remainder.value_s64 = 0;
            } else {
                result_quotient.value_s64 = (s64)(dividend / divisor);;
                result_remainder.value_s64 = (s64)(dividend % divisor);
            }
            break;
        }
    }

    host_emit_mov_mem_imm(Dst, (uintptr_t)&N64CPU.mult_lo, result_quotient, VALUE_TYPE_S64);
    host_emit_mov_mem_imm(Dst, (uintptr_t)&N64CPU.mult_hi, result_remainder, VALUE_TYPE_S64);
}

void compile_ir_multiply(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->mult_div.operand1) && is_constant(instr->mult_div.operand2)) {
        evaluate_const_multiply(Dst, instr->mult_div.operand1->set_constant, instr->mult_div.operand2->set_constant, instr->mult_div.mult_div_type);
    } else if (instr_valid_immediate(instr->mult_div.operand1)) {
        host_emit_mult_reg_imm(Dst, instr->mult_div.operand2->reg_alloc, instr->mult_div.operand1->set_constant, instr->mult_div.mult_div_type);
    } else if (instr_valid_immediate(instr->mult_div.operand2)) {
        host_emit_mult_reg_imm(Dst, instr->mult_div.operand1->reg_alloc, instr->mult_div.operand2->set_constant, instr->mult_div.mult_div_type);
    } else {
        host_emit_mult_reg_reg(Dst, instr->mult_div.operand1->reg_alloc, instr->mult_div.operand2->reg_alloc, instr->mult_div.mult_div_type);
    }
}

void compile_ir_divide(dasm_State** Dst, ir_instruction_t* instr) {
    if (is_constant(instr->mult_div.operand1) && is_constant(instr->mult_div.operand2)) {
        evaluate_const_divide(Dst, instr->mult_div.operand1->set_constant, instr->mult_div.operand2->set_constant, instr->mult_div.mult_div_type);
    } else if (instr_valid_immediate(instr->mult_div.operand1)) {
        host_emit_div_imm_reg(Dst, instr->mult_div.operand1->set_constant, instr->mult_div.operand2->reg_alloc, instr->mult_div.mult_div_type);
    } else if (instr_valid_immediate(instr->mult_div.operand2)) {
        host_emit_div_reg_imm(Dst, instr->mult_div.operand1->reg_alloc, instr->mult_div.operand2->set_constant, instr->mult_div.mult_div_type);
    } else {
        host_emit_div_reg_reg(Dst, instr->mult_div.operand1->reg_alloc, instr->mult_div.operand2->reg_alloc, instr->mult_div.mult_div_type);
    }
}

void compile_ir_eret(dasm_State** Dst) {
    ir_context.block_end_pc_compiled = true;
    host_emit_eret(Dst);
}

void compile_ir_call(dasm_State** Dst, ir_instruction_t* instr) {
    for (int i = 0; i < instr->call.num_args; i++) {
        val_to_func_arg(Dst, instr->call.arguments[i], i);
    }
    host_emit_call(Dst, instr->call.function);
}

void compile_ir_mov_reg_type(dasm_State** Dst, ir_instruction_t* instr) {
    ir_register_type_t dest_type = instr->mov_reg_type.new_type;

    if (is_constant(instr->mov_reg_type.value)) {
        logfatal("mov reg type with const arg");
    } else {
        ir_register_type_t source_type = instr->mov_reg_type.value->reg_alloc.type;
        switch (source_type) {
            case REGISTER_TYPE_NONE:
                logfatal("mov_reg_type with source type REGISTER_TYPE_NONE");
                break;
            case REGISTER_TYPE_GPR:
                switch (dest_type) {
                    case REGISTER_TYPE_NONE:
                        logfatal("mov_reg_type with source type REGISTER_TYPE_GPR and dest type REGISTER_TYPE_NONE");
                        break;
                    case REGISTER_TYPE_GPR:
                        logfatal("mov_reg_type with source type REGISTER_TYPE_GPR and dest type REGISTER_TYPE_GPR");
                        break;
                    case REGISTER_TYPE_FGR_32:
                    case REGISTER_TYPE_FGR_64:
                        host_emit_mov_fgr_gpr(Dst, instr->reg_alloc, instr->mov_reg_type.value->reg_alloc, instr->mov_reg_type.size);
                        break;
                }
                break;
            case REGISTER_TYPE_FGR_32:
                switch (dest_type) {
                    case REGISTER_TYPE_NONE:
                        logfatal("mov_reg_type with source type REGISTER_TYPE_FGR_32 and dest type REGISTER_TYPE_NONE");
                        break;
                    case REGISTER_TYPE_GPR:
                        host_emit_mov_gpr_fgr(Dst, instr->reg_alloc, instr->mov_reg_type.value->reg_alloc, instr->mov_reg_type.size);
                        break;
                    case REGISTER_TYPE_FGR_32:
                        logfatal("mov_reg_type with source type REGISTER_TYPE_FGR_32 and dest type REGISTER_TYPE_FGR_32");
                        break;
                    case REGISTER_TYPE_FGR_64:
                        logfatal("mov_reg_type with source type REGISTER_TYPE_FGR_32 and dest type REGISTER_TYPE_FGR_64");
                        break;
                }
                break;
            case REGISTER_TYPE_FGR_64:
                switch (dest_type) {
                    case REGISTER_TYPE_NONE:
                        logfatal("mov_reg_type REGISTER_TYPE_FGR_64 -> REGISTER_TYPE_NONE");
                        break;
                    case REGISTER_TYPE_GPR:
                        host_emit_mov_gpr_fgr(Dst, instr->reg_alloc, instr->mov_reg_type.value->reg_alloc, instr->mov_reg_type.size);
                        break;
                    case REGISTER_TYPE_FGR_32:
                        logfatal("mov_reg_type REGISTER_TYPE_FGR_64 -> REGISTER_TYPE_FGR_32");
                        break;
                    case REGISTER_TYPE_FGR_64:
                        logfatal("mov_reg_type REGISTER_TYPE_FGR_64 -> REGISTER_TYPE_FGR_64");
                        break;
                }
                break;
        }
    }
}

void compile_ir_set_float_constant(dasm_State** Dst, ir_instruction_t* instr) {
    ir_set_float_constant_t fc = instr->set_float_constant;
    ir_set_constant_t c;
    switch (fc.format) {
        case FLOAT_VALUE_TYPE_INVALID:
            logfatal("compile_ir_set_float_constant FLOAT_VALUE_TYPE_INVALID");
            break;
        case FLOAT_VALUE_TYPE_WORD:
        case FLOAT_VALUE_TYPE_SINGLE:
            c.type = VALUE_TYPE_U32;
            c.value_u32 = fc.value_word;
            break;
        case FLOAT_VALUE_TYPE_LONG:
        case FLOAT_VALUE_TYPE_DOUBLE:
            c.type = VALUE_TYPE_U32;
            c.value_u64 = fc.value_long;
            break;
    }

    host_emit_mov_reg_imm(Dst, TMPREG1_ALLOC, c);
    host_emit_mov_fgr_gpr(Dst, instr->reg_alloc, TMPREG1_ALLOC, c.type);
}

void compile_ir_float_convert(dasm_State** Dst, ir_instruction_t* instr) {
    switch (instr->float_convert.mode) {
        case FLOAT_CONVERT_MODE_CONVERT:
            host_emit_float_convert_reg_reg(Dst, instr->float_convert.from_type, instr->float_convert.value->reg_alloc, instr->float_convert.to_type, instr->reg_alloc);
            break;
        case FLOAT_CONVERT_MODE_TRUNC:
            host_emit_float_trunc_reg_reg(Dst, instr->float_convert.from_type, instr->float_convert.value->reg_alloc, instr->float_convert.to_type, instr->reg_alloc);
            break;
        case FLOAT_CONVERT_MODE_ROUND:
            host_emit_float_round_reg_reg(Dst, instr->float_convert.from_type, instr->float_convert.value->reg_alloc, instr->float_convert.to_type, instr->reg_alloc);
            break;
        case FLOAT_CONVERT_MODE_FLOOR:
            host_emit_float_floor_reg_reg(Dst, instr->float_convert.from_type, instr->float_convert.value->reg_alloc, instr->float_convert.to_type, instr->reg_alloc);
            break;
    }
}

void compile_ir_float_divide(dasm_State** Dst, ir_instruction_t* instr) {
    unimplemented(is_constant(instr->float_bin_op.operand1), "float div with constant dividend");
    unimplemented(is_constant(instr->float_bin_op.operand2), "float div with constant divisor");
    host_emit_mov_fgr_fgr(Dst, instr->reg_alloc, instr->float_bin_op.operand1->reg_alloc, instr->float_bin_op.format);
    host_emit_float_div_reg_reg(Dst, instr->reg_alloc, instr->float_bin_op.operand2->reg_alloc, instr->float_bin_op.format);
}

void compile_ir_float_multiply(dasm_State** Dst, ir_instruction_t* instr) {
    unimplemented(is_constant(instr->float_bin_op.operand1), "float mult with constant multiplicand1");
    unimplemented(is_constant(instr->float_bin_op.operand2), "float mult with constant multiplicand2");
    host_emit_mov_fgr_fgr(Dst, instr->reg_alloc, instr->float_bin_op.operand1->reg_alloc, instr->float_bin_op.format);
    host_emit_float_mult_reg_reg(Dst, instr->reg_alloc, instr->float_bin_op.operand2->reg_alloc, instr->float_bin_op.format);
}

void compile_ir_float_add(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_mov_fgr_fgr(Dst, instr->reg_alloc, instr->float_bin_op.operand1->reg_alloc, instr->float_bin_op.format);
    host_emit_float_add_reg_reg(Dst, instr->reg_alloc, instr->float_bin_op.operand2->reg_alloc, instr->float_bin_op.format);
}

void compile_ir_float_sub(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_mov_fgr_fgr(Dst, instr->reg_alloc, instr->float_bin_op.operand1->reg_alloc, instr->float_bin_op.format);
    host_emit_float_sub_reg_reg(Dst, instr->reg_alloc, instr->float_bin_op.operand2->reg_alloc, instr->float_bin_op.format);
}

void compile_ir_float_sqrt(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_float_sqrt_reg_reg(Dst, instr->reg_alloc, instr->float_unary_op.operand->reg_alloc, instr->float_unary_op.format);
}

void compile_ir_float_neg(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_float_neg_reg_reg(Dst, instr->reg_alloc, instr->float_unary_op.operand->reg_alloc, instr->float_unary_op.format);
}

void compile_ir_float_abs(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_float_abs_reg_reg(Dst, instr->reg_alloc, instr->float_unary_op.operand->reg_alloc, instr->float_unary_op.format);
}

void compile_ir_float_check_condition(dasm_State** Dst, ir_instruction_t* instr) {
    host_emit_float_cmp(Dst, instr->float_check_condition.condition, instr->float_check_condition.format, instr->float_check_condition.operand1->reg_alloc, instr->float_check_condition.operand2->reg_alloc);
}

void compile_ir_interpreter_fallback(dasm_State** Dst, ir_instruction_t* instr, int extra_cycles) {
    switch (instr->interpreter_fallback.type) {
        case INTERPRETER_FALLBACK_FOR_INSTRUCTIONS:
            logfatal("Interpreter fallback for instructions");
            break;
        case INTERPRETER_FALLBACK_UNTIL_NO_BRANCH:
            host_emit_interpreter_fallback_until_no_branch(Dst, extra_cycles);
            break;
    }
}

void v2_emit_instr(dasm_State** Dst, ir_instruction_t* instr) {
    switch (instr->type) {
        case IR_NOP: break;
        case IR_SET_CONSTANT:
            // Only load into a host register if it was determined by the allocator that we need to
            // Otherwise, it can be used as an immediate when needed
            if (instr->reg_alloc.allocated) {
                compile_ir_set_constant(Dst, instr);
            }
            break;
        case IR_OR:
            compile_ir_or(Dst, instr);
            break;
        case IR_XOR:
            compile_ir_xor(Dst, instr);
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
        case IR_SUB:
            compile_ir_sub(Dst, instr);
            break;
        case IR_STORE:
            compile_ir_store(Dst, instr);
            break;
        case IR_LOAD:
            compile_ir_load(Dst, instr);
            break;
        case IR_GET_PTR:
            compile_ir_get_ptr(Dst, instr);
            break;
        case IR_SET_PTR:
            compile_ir_set_ptr(Dst, instr);
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
        case IR_COND_BLOCK_EXIT:
            compile_ir_cond_block_exit(Dst, instr);
            break;
        case IR_MULTIPLY:
            compile_ir_multiply(Dst, instr);
            break;
        case IR_DIVIDE:
            compile_ir_divide(Dst, instr);
            break;
        case IR_ERET:
            compile_ir_eret(Dst);
            break;
        case IR_CALL:
            compile_ir_call(Dst, instr);
            break;
        case IR_MOV_REG_TYPE:
            compile_ir_mov_reg_type(Dst, instr);
            break;
        case IR_SET_FLOAT_CONSTANT:
            compile_ir_set_float_constant(Dst, instr);
            break;
        case IR_FLOAT_CONVERT:
            compile_ir_float_convert(Dst, instr);
            break;
        case IR_FLOAT_DIVIDE:
            compile_ir_float_divide(Dst, instr);
            break;
        case IR_FLOAT_MULTIPLY:
            compile_ir_float_multiply(Dst, instr);
            break;
        case IR_FLOAT_ADD:
            compile_ir_float_add(Dst, instr);
            break;
        case IR_FLOAT_SUB:
            compile_ir_float_sub(Dst, instr);
            break;
        case IR_FLOAT_SQRT:
            compile_ir_float_sqrt(Dst, instr);
            break;
        case IR_FLOAT_NEG:
            compile_ir_float_neg(Dst, instr);
            break;
        case IR_FLOAT_ABS:
            compile_ir_float_abs(Dst, instr);
            break;
        case IR_FLOAT_CHECK_CONDITION:
            compile_ir_float_check_condition(Dst, instr);
            break;
        case IR_INTERPRETER_FALLBACK:
            compile_ir_interpreter_fallback(Dst, instr, temp_code_len);
            break;
    }
}

void v2_emit_block(n64_dynarec_block_t* block, u32 physical_address) {
    dasm_State** Dst = v2_block_header();

    if (should_break(physical_address)) {
        host_emit_debugbreak(Dst);
    }
    ir_instruction_t* instr = ir_context.ir_cache_head;
    while (instr) {
        v2_emit_instr(Dst, instr);
        instr = instr->next;
    }

    if (!ir_context.block_end_pc_compiled && temp_code_len > 0) {
        logfatal("TODO: emit end of block PC");
    }
    v2_end_block(Dst, temp_code_len);
    size_t code_size = v2_link(Dst);
#ifdef N64_LOG_COMPILATIONS
    printf("Generated %zu bytes of code\n", code_size);
#endif
    block->guest_size = temp_code_len * 4;
    block->host_size = code_size;
    block->run = (int(*)(r4300i_t *))dynarec_bumpalloc(code_size);;

    v2_encode(Dst, (u8*)block->run);
    v2_dasm_free();
}

bool detect_idle_loop(u64 virtual_address) {
    if (!v2_idle_loop_detection_enabled) {
        return false;
    }

    if (temp_code_len == 2 && temp_code[1].instr.raw == 0x00000000) {
        // b -1
        // nop
        if (temp_code[0].instr.raw == 0x1000FFFF) {
            return true;
        }

        // j (self)
        // nop
        if (temp_code[0].instr.op == OPC_J && temp_code[0].instr.j.target == ((virtual_address >> 2) & 0x3FFFFFF)) {
            return true;
        }
    }


    return false;
}

int idle_loop_replacement() {
    // +1 just in case we're executing this with 0 cycles until the next event
    u64 ticks_to_skip = scheduler_ticks_until_next_event() + 1;
    return ticks_to_skip;
}

void v2_compile_new_block(
        n64_dynarec_block_t* block,
        bool* code_mask,
        u64 virtual_address,
        u32 physical_address) {

    fill_temp_code(virtual_address, physical_address, code_mask);

    if (detect_idle_loop(virtual_address)) {
        block->run = idle_loop_replacement;
        block->guest_size = 0;
        block->host_size = 0;
        return;
    }

    ir_context_reset();
    ir_context.block_start_virtual = virtual_address;
    ir_context.block_start_physical = physical_address;
#ifdef N64_LOG_COMPILATIONS
    printf("Translating to IR:\n");
#endif

    // If the block ends with a branch, don't include it in the block, and instead fall back to the interpreter.
    // The block should only ever end up on a branch if it's cut off by a page boundary.
    bool block_ends_with_branch = LAST_INSTR_IS_BRANCH;

    // Trim all branches off the end of the block (they will be replaced by the interpreter fallback)
    while (LAST_INSTR_IS_BRANCH) {
        temp_code_len--;
    }

    for (int i = 0; i < temp_code_len; i++) {
        u64 instr_virtual_address = virtual_address + (i << 2);
        u32 instr_physical_address = physical_address + (i << 2);
        emit_instruction_ir(temp_code[i].instr, i, instr_virtual_address, instr_physical_address);
    }

    if (!ir_context.block_end_pc_ir_emitted && temp_code_len > 0) {
        ir_instruction_t* end_pc = ir_emit_set_constant_64(virtual_address + (temp_code_len << 2), NO_GUEST_REG);
        ir_emit_set_block_exit_pc(end_pc);
    }

    ir_optimize_flush_guest_regs();

    if (block_ends_with_branch) {
        ir_emit_interpreter_fallback_until_no_delay_slot();
    }
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
    v2_emit_block(block, physical_address);
    if (block->run == NULL) {
        logfatal("Failed to emit block");
    }
#ifdef N64_DEBUG_MODE
    if (should_break(physical_address)) {
        print_multi_host((uintptr_t)block->run, (u8*)block->run, block->host_size);
        if (physical_address < N64_RDRAM_SIZE) {
            print_multi_guest((uintptr_t)physical_address, &n64sys.mem.rdram[physical_address], block->guest_size);
        }
    }
#endif
#ifdef N64_LOG_COMPILATIONS
    print_multi_host((uintptr_t)block->run, (u8*)block->run, block->host_size);
#endif
}

void v2_compiler_init() {
    uintptr_t run_block_code_ptr = (uintptr_t)run_block_codecache;
    if ((run_block_code_ptr & (4096 - 1)) != 0) {
        logfatal("Run block code pointer not page aligned!");
    }
    n64dynarec.run_block = (int (*)(u64)) run_block_code_ptr;
    mprotect_rwx((u8*)&run_block_codecache, DISPATCHER_CODE_SIZE, "run block");

    N64CPU.s_mask[0] = 0xFFFFFFFF;
    N64CPU.d_mask[0] = 0xFFFFFFFFFFFFFFFF;

    N64CPU.s_neg[0] = (u32)1 << 31;
    N64CPU.d_neg[0] = (u64)1 << 63;

    N64CPU.s_abs[0] = (u32) ~N64CPU.s_neg[0];
    N64CPU.d_abs[0] = (u64) ~N64CPU.d_neg[0];

    N64CPU.int64_min = INT64_MIN;

    {
        dasm_State **Dst = v2_emit_run_block();
        size_t dispatcher_code_size = v2_link(Dst);

        if (dispatcher_code_size >= DISPATCHER_CODE_SIZE) {
            logfatal("Compiled dispatcher too large!");
        }

        v2_encode(Dst, (u8 *) &run_block_codecache);
        v2_dasm_free();
    }
}

void v2_set_idle_loop_detection_enabled(bool enabled) {
    v2_idle_loop_detection_enabled = enabled;
}
