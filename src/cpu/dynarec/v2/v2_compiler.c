#include <mem/n64bus.h>
#include <disassemble.h>
#include "v2_compiler.h"

#include "instruction_category.h"
#include "ir_emitter.h"
#include "ir_context.h"

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

void v2_compile_new_block(
        n64_dynarec_block_t* block,
        bool* code_mask,
        u64 virtual_address,
        u32 physical_address) {

    fill_temp_code(virtual_address, physical_address, code_mask);
    ir_context_reset();
    int last_ir_index = 0;
    printf("Translating to IR:\n");
    for (int i = 0; i < temp_code_len; i++) {
        u64 instr_virtual_address = virtual_address + (i << 2);
        u32 instr_physical_address = physical_address + (i << 2);
        emit_instruction_ir(temp_code[i].instr, instr_virtual_address, instr_physical_address);

        // TODO when we can compile full blocks, move this to a separate for loop
        while (last_ir_index < ir_context.ir_cache_index) {
            static char buf[100];
            ir_instr_to_string(last_ir_index, buf, 100);
            printf("%s\n", buf);
            last_ir_index++;
        }
    }
    logfatal("Emitted IR for a block. It's time to optimize/emit");
}

void v2_compiler_init() {

}
