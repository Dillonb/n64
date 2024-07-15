#include <log.h>
#include <perf_map_file.h>
#include <rsp.h>
#include "rsp_dynarec.h"
#include "v1/v1_emitter.h"
#include "dynarec_memory_management.h"
#ifdef N64_HAVE_SSE
#include <emmintrin.h>
#include <smmintrin.h>
#endif

size_t rsp_link(dasm_State** d) {
    size_t code_size;
    dasm_link(d, &code_size);
    return code_size;
}

void* rsp_link_and_encode(dasm_State** Dst, size_t* code_size_result) {
    size_t code_size = rsp_link(Dst);
#ifdef N64_LOG_COMPILATIONS
    printf("Generated %ld bytes of RSP code\n", code_size);
#endif
    void* buf = rsp_dynarec_bumpalloc(code_size);
    dasm_encode(Dst, buf);

    if (code_size_result) {
        *code_size_result = code_size;
    }

    return buf;
}

#define NEXT(address) ((address + 4) & 0xFFF)

void compile_new_rsp_block(rsp_dynarec_block_t* block, u16 address, rsp_code_overlay_t* current_overlay) {
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
        current_overlay->code[address >> 2] = instr.raw;
        current_overlay->code_mask[address >> 2] = 0xFFFFFFFF;

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
    size_t code_size;
    void* compiled = rsp_link_and_encode(Dst, &code_size);
    v1_dasm_free();

    block->run = compiled;

    char block_name[500];
    snprintf(block_name, 500, "rsp_jit_block_%04X", address);

    n64_perf_map_file_write((uintptr_t)compiled, code_size, block_name);
}

int rsp_missing_block_handler() {
    u32 pc = N64RSP.pc & 0x3FF;
    rsp_code_overlay_t* current_overlay = &N64RSPDYNAREC->code_overlays[N64RSPDYNAREC->selected_code_overlay];
    rsp_dynarec_block_t* block = &current_overlay->blockcache[pc];
    compile_new_rsp_block(block, (N64RSP.pc << 2) & 0xFFF, current_overlay);
    return block->run(&N64RSP);
}

void reset_rsp_dynarec_code_overlay(rsp_code_overlay_t* overlay) {
    for (int i = 0; i < RSP_BLOCKCACHE_SIZE; i++) {
        overlay->blockcache[i].run = rsp_missing_block_handler;
        overlay->code[i] = 0;
        overlay->code_mask[i] = 0;
    }
}

void reset_rsp_dynarec_code_overlays(rsp_dynarec_t* dynarec) {
    for (int i = 0; i < RSP_NUM_CODE_OVERLAYS; i++) {
            reset_rsp_dynarec_code_overlay(&dynarec->code_overlays[i]);
    }
}

rsp_dynarec_t* rsp_dynarec_init(u8* codecache, size_t codecache_size) {
    rsp_dynarec_t* dynarec = calloc(1, sizeof(rsp_dynarec_t));

    dynarec->codecache_size = codecache_size;
    dynarec->codecache_used = 0;

    reset_rsp_dynarec_code_overlays(dynarec);

    dynarec->codecache = codecache;

    return dynarec;
}

bool code_overlay_matches(int index) {
    rsp_code_overlay_t* overlay = &N64RSPDYNAREC->code_overlays[index];

#ifdef N64_HAVE_SSE

    const s128* mask_arr = (s128*)overlay->code_mask;
    const s128* code_arr = (s128*)overlay->code;
    const s128* imem_arr = (s128*)N64RSP.sp_imem;

    for (int i = 0; i < RSP_BLOCKCACHE_SIZE / 4; i++) {
        const s128 mask = _mm_loadu_si128(mask_arr + i);
        const s128 code = _mm_loadu_si128(code_arr + i);
        const s128 imem = _mm_loadu_si128(imem_arr + i);
        // equivalent to if ((code ^ imem) & mask)
        if (!_mm_testz_si128(_mm_xor_si128(code, imem), mask)) {
            return false;
        }
    }

#else

    for (int i = 0; i < RSP_BLOCKCACHE_SIZE; i++) {
        if (overlay->code_mask[i]) {
            if (overlay->code[i] != word_from_byte_array(n64rsp.sp_imem, i << 2)) {
                return false;
            }
        }
    }

#endif
    return true;
}

int rsp_dynarec_step() {
    if (N64RSPDYNAREC->dirty) {
        // see if we match any existing blocks, if yes, switch to the first matching block we find.
        // if no, allocate a new one.
        // if we're out of blocks, choose a random one to overwrite and use.
        bool found_match = false;
        for (int i = 0; i < N64RSPDYNAREC->code_overlays_allocated && !found_match; i++) {
            if (code_overlay_matches(i)) {
                found_match = true;
                N64RSPDYNAREC->selected_code_overlay = i;
            }
        }

        if (!found_match) {
            int new_code_overlay = N64RSPDYNAREC->code_overlays_allocated;
            if (new_code_overlay >= RSP_NUM_CODE_OVERLAYS) {
                new_code_overlay = rand() % RSP_NUM_CODE_OVERLAYS;
                logwarn("RSP: Out of code overlays! Selecting %d randomly", new_code_overlay);
            } else {
                N64RSPDYNAREC->code_overlays_allocated++;
                logwarn("RSP: Allocated a new code overlay. Allocated %d so far.", N64RSPDYNAREC->code_overlays_allocated);
            }
            reset_rsp_dynarec_code_overlay(&N64RSPDYNAREC->code_overlays[new_code_overlay]);
            N64RSPDYNAREC->selected_code_overlay = new_code_overlay;
        }
        N64RSPDYNAREC->dirty = false;
    }

    return N64RSPDYNAREC->code_overlays[N64RSPDYNAREC->selected_code_overlay].blockcache[N64RSP.pc & 0x3FF].run(&N64RSP);
}
