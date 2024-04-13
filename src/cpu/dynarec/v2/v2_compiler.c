#include "v2_compiler.h"

#include <log.h>
#include <mem/n64bus.h>
#include <disassemble.h>
#include <dynarec/dynarec_memory_management.h>
#include <r4300i.h>
#include <r4300i_register_access.h>
#include "v2_compiler_platformspecific.h"
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

bool should_break(u32 address) {
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

static bool v2_idle_loop_detection_enabled = true;

int temp_code_len = 0;
source_instruction_t temp_code[TEMP_CODE_SIZE];
u64 temp_code_vaddr = 0;

#define LAST_INSTR_CATEGORY (temp_code[temp_code_len - 1].category)
#define LAST_INSTR_IS_BRANCH ((temp_code_len > 0) && ((LAST_INSTR_CATEGORY == BRANCH) || (LAST_INSTR_CATEGORY == BRANCH_LIKELY)))

u64 v2_get_last_compiled_block() {
    return temp_code_vaddr;
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

    bool cached; // ignored for now
    if (resolve_virtual_address(virtual, bus_access, &cached, &physical)) {
        return physical;
    } else {
        on_tlb_exception(virtual);
        u32 code = get_tlb_exception_code(N64CP0.tlb_error, bus_access);
        r4300i_handle_exception(except_pc, code, 0);
        return 1ull << 32;
    }
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
    N64CPU.s_mask[0] = 0xFFFFFFFF;
    N64CPU.d_mask[0] = 0xFFFFFFFFFFFFFFFF;

    N64CPU.s_neg[0] = (u32)1 << 31;
    N64CPU.d_neg[0] = (u64)1 << 63;

    N64CPU.s_abs[0] = (u32) ~N64CPU.s_neg[0];
    N64CPU.d_abs[0] = (u64) ~N64CPU.d_neg[0];

    N64CPU.int64_min = INT64_MIN;

    v2_compiler_init_platformspecific();
}

void v2_set_idle_loop_detection_enabled(bool enabled) {
    v2_idle_loop_detection_enabled = enabled;
}