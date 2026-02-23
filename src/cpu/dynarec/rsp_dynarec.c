#include <log.h>
#include <mips_instruction_decode.h>
#include <perf_map_file.h>
#include <rsp.h>
#include "rsp_dynarec.h"
#include "jit_rs.h"

#ifdef N64_HAVE_SSE
#ifdef N64_USE_NEON
#include <sse2neon.h>
#else
#include <emmintrin.h>
#include <smmintrin.h>
#endif
#endif

#define NEXT(address) ((address + 4) & 0xFFF)
void compile_new_rsp_block(rsp_dynarec_block_t* block, u16 address, rsp_code_overlay_t* current_overlay) {
    rs_jit_compile_new_rsp_block(block, address, current_overlay, &N64RSP);
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
