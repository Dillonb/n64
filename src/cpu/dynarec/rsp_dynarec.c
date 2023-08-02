#include <log.h>
#include <rsp.h>
#include "rsp_dynarec.h"
#include "v1/v1_emitter.h"
#include "dynarec_memory_management.h"

void* rsp_link_and_encode(dasm_State** Dst) {
    size_t code_size;
    dasm_link(Dst, &code_size);
#ifdef N64_LOG_COMPILATIONS
    printf("Generated %ld bytes of RSP code\n", code_size);
#endif
    void* buf = rsp_dynarec_bumpalloc(code_size);
    dasm_encode(Dst, buf);

    return buf;
}

#define NEXT(address) ((address + 4) & 0xFFF)

void compile_new_rsp_block(rsp_dynarec_block_t* block, u16 address) {
    dasm_State** Dst = v1_block_header();

    int block_length = 0;
    int block_extra_cycles = 0;
    bool should_continue_block = true;
    u32 extra_cycles = 0;
    int instructions_left_in_block = -1;
    bool branch_in_block = false;

    dynarec_instruction_category_t prev_instr_category = NORMAL;

    do {
        static mips_instruction_t instr;
        instr.raw = word_from_byte_array(N64RSP.sp_imem, address);

        u16 next_address = NEXT(address);

        instructions_left_in_block--;

        dynarec_ir_t* ir = rsp_instruction_ir(instr, address);
        if (is_branch(ir->category)) {
            flush_rsp_pc(Dst, next_address >> 2);
            flush_rsp_next_pc(Dst, NEXT(next_address) >> 2);
        }

        //advance_rsp_pc(Dst);

        ir->compiler(Dst, instr, address, NULL, 0, &extra_cycles);
        block_length++;
        block_extra_cycles += extra_cycles;

        switch (ir->category) {
            case NORMAL:
                should_continue_block = instructions_left_in_block != 0;
                break;
            case BRANCH:
                advance_rsp_pc(Dst);
                branch_in_block = true;
                if (prev_instr_category == BRANCH) {
                    // Check if the previous branch was taken.

                    // If the last branch wasn't taken, we can treat this the same as if the previous instruction wasn't a branch
                    // just set the N64CPU.last_branch_taken to N64CPU.branch_taken and execute the next instruction.

                    // emit:
                    // if (!N64CPU.last_branch_taken) N64CPU.last_branch_taken = N64CPU.branch_taken;
                    logfatal("Branch in a branch delay slot");
                } else {
                    // If the last instruction wasn't a branch, no special behavior is needed. Just set up some state in case the next one is.
                    // emit:
                    // N64CPU.last_branch_taken = N64CPU.branch_taken;
                    //logfatal("unimp");
                }

                should_continue_block = true;
                instructions_left_in_block = 1; // emit delay slot

                break;

            case BLOCK_ENDER:
                should_continue_block = false;
                break;

            default:
                logfatal("Unknown dynarec instruction type");
        }
        address = next_address;
        prev_instr_category = ir->category;
    } while(should_continue_block);

    if (!branch_in_block) {
        flush_rsp_pc(Dst, address >> 2);
        flush_rsp_next_pc(Dst, NEXT(address) >> 2);
    }

    end_rsp_block(Dst, block_length + block_extra_cycles);
    void* compiled = rsp_link_and_encode(Dst);
    v1_dasm_free();

    block->run = compiled;
}

int rsp_missing_block_handler() {
    u32 pc = N64RSP.pc & 0x3FF;
    rsp_dynarec_block_t* block = &N64RSPDYNAREC->blockcache[pc];
    compile_new_rsp_block(block, (N64RSP.pc << 2) & 0xFFF);
    return block->run(&N64RSP);
}

rsp_dynarec_t* rsp_dynarec_init(u8* codecache, size_t codecache_size) {
    rsp_dynarec_t* dynarec = calloc(1, sizeof(rsp_dynarec_t));

    dynarec->codecache_size = codecache_size;
    dynarec->codecache_used = 0;

    for (int i = 0; i < RSP_BLOCKCACHE_SIZE; i++) {
        dynarec->blockcache[i].run = rsp_missing_block_handler;
    }

    dynarec->codecache = codecache;

    return dynarec;
}

int rsp_dynarec_step() {
    return N64RSPDYNAREC->blockcache[N64RSP.pc & 0x3FF].run(&N64RSP);
}
