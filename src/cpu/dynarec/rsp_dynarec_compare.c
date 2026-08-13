#include "rsp_dynarec_compare.h"

#include <stdlib.h>
#include <string.h>

#include <log.h>

#include "rsp_dynarec.h"
#include "../rsp.h"

// Runs each block twice: once on the dynarec, then again on the interpreter from the same starting
// state, and diffs the results. The interpreter's state is the one kept, so a bug in one block can't
// cascade and every divergent block gets reported rather than just the first.
//
// Re-running a block repeats its side effects, which is fine on the RSP because they're all
// idempotent: DMA copies the same bytes again, and raising the SP interrupt just sets a bit.

#define MAX_REPORTS 20
#define RSP_PC_MASK 0x3FF

static int reports = 0;

bool rsp_compare_enabled() {
    static int enabled = -1;
    if (enabled < 0) {
        enabled = getenv("RSP_COMPARE") != NULL;
        if (enabled) {
            logalways("RSP dynarec/interpreter comparison enabled");
        }
    }
    return enabled;
}

static bool fatal_on_diff() {
    static int fatal = -1;
    if (fatal < 0) {
        fatal = getenv("RSP_COMPARE_FATAL") != NULL;
    }
    return fatal;
}

static void print_vu_reg(const char* name, int index, const vu_reg_t* jit, const vu_reg_t* interp) {
    printf("    %s", name);
    if (index >= 0) {
        printf("[%02d]", index);
    }
    printf(" jit=");
    for (int i = 0; i < 16; i++) {
        printf("%02X", jit->bytes[i]);
    }
    printf(" interp=");
    for (int i = 0; i < 16; i++) {
        printf("%02X", interp->bytes[i]);
    }
    printf("\n");
}

static void print_memory_diff(const char* name, const u8* jit, const u8* interp, size_t size) {
    int shown = 0;
    for (size_t i = 0; i < size && shown < 8; i++) {
        if (jit[i] != interp[i]) {
            printf("    %s[0x%03X] jit=%02X interp=%02X\n", name, (unsigned)i, jit[i], interp[i]);
            shown++;
        }
    }
    if (shown == 8) {
        printf("    %s: ... more differences not shown\n", name);
    }
}

#define DIFF_SCALAR_MASKED(field, mask, fmt) \
    do { \
        if ((jit->field & (mask)) != (interp->field & (mask))) { \
            printf("    " #field " jit=" fmt " interp=" fmt "\n", jit->field & (mask), interp->field & (mask)); \
            differs = true; \
        } \
    } while (0)

#define DIFF_SCALAR(field, fmt) DIFF_SCALAR_MASKED(field, ~0u, fmt)

#define DIFF_VU(field) \
    do { \
        if (memcmp(&jit->field, &interp->field, sizeof(vu_reg_t)) != 0) { \
            print_vu_reg(#field, -1, &jit->field, &interp->field); \
            differs = true; \
        } \
    } while (0)

#define DIFF_MEMORY(field) \
    do { \
        if (memcmp(jit->field, interp->field, sizeof(jit->field)) != 0) { \
            print_memory_diff(#field, jit->field, interp->field, sizeof(jit->field)); \
            differs = true; \
        } \
    } while (0)

// Compares everything the two engines are both responsible for. prev_pc and the icache are skipped
// because only the interpreter maintains them, and the dynarec pointer is not state.
static bool report_differences(const rsp_t* jit, const rsp_t* interp, u16 block_pc, int instructions) {
    bool differs = false;

    printf("=================== RSP divergence ====================\n");
    printf("  block at 0x%03X (%d instructions)\n", block_pc << 2, instructions);

    for (int i = 0; i < 32; i++) {
        if (jit->gpr[i] != interp->gpr[i]) {
            printf("    gpr[%02d] jit=%08X interp=%08X\n", i, jit->gpr[i], interp->gpr[i]);
            differs = true;
        }
    }

    for (int i = 0; i < 32; i++) {
        if (memcmp(&jit->vu_regs[i], &interp->vu_regs[i], sizeof(vu_reg_t)) != 0) {
            print_vu_reg("vu_regs", i, &jit->vu_regs[i], &interp->vu_regs[i]);
            differs = true;
        }
    }

    // JIT doesn't bother masking the PC
    DIFF_SCALAR_MASKED(pc, RSP_PC_MASK, "%03X");
    DIFF_SCALAR_MASKED(next_pc, RSP_PC_MASK, "%03X");
    DIFF_SCALAR(status.raw, "%08X");

    DIFF_SCALAR(io.mem_addr.raw, "%08X");
    DIFF_SCALAR(io.dram_addr.raw, "%08X");
    DIFF_SCALAR(io.shadow_mem_addr.raw, "%08X");
    DIFF_SCALAR(io.shadow_dram_addr.raw, "%08X");
    DIFF_SCALAR(io.dma.raw, "%08X");

    DIFF_VU(acc.h);
    DIFF_VU(acc.m);
    DIFF_VU(acc.l);
    DIFF_VU(vcc.l);
    DIFF_VU(vcc.h);
    DIFF_VU(vco.l);
    DIFF_VU(vco.h);
    DIFF_VU(vce);

    DIFF_SCALAR(divin, "%04X");
    DIFF_SCALAR(divout, "%04X");
    DIFF_SCALAR(divin_loaded, "%d");
    DIFF_SCALAR(semaphore_held, "%d");

    DIFF_MEMORY(sp_dmem);
    DIFF_MEMORY(sp_imem);

    if (!differs) {
        // Something outside the compared fields changed. Better to say so than to claim a match.
        printf("    (no compared field differs)\n");
    }
    printf("=======================================================\n");
    return differs;
}

int rsp_dynarec_step_compare() {
    static rsp_t before;
    static rsp_t after_jit;

    u16 block_pc = N64RSP.pc & 0x3FF;

    before = N64RSP;
    int instructions = rsp_dynarec_step();
    after_jit = N64RSP;

    // Rewind and run the same instructions through the interpreter.
    N64RSP = before;
    for (int i = 0; i < instructions; i++) {
        rsp_step();
    }

    if (memcmp(&after_jit.gpr, &N64RSP.gpr, sizeof(N64RSP.gpr)) != 0
        || memcmp(&after_jit.vu_regs, &N64RSP.vu_regs, sizeof(N64RSP.vu_regs)) != 0
        || memcmp(&after_jit.sp_dmem, &N64RSP.sp_dmem, sizeof(N64RSP.sp_dmem)) != 0
        || memcmp(&after_jit.acc, &N64RSP.acc, sizeof(N64RSP.acc)) != 0
        || (after_jit.pc & RSP_PC_MASK) != (N64RSP.pc & RSP_PC_MASK)
        || (after_jit.next_pc & RSP_PC_MASK) != (N64RSP.next_pc & RSP_PC_MASK)) {

        if (reports < MAX_REPORTS) {
            reports++;
            report_differences(&after_jit, &N64RSP, block_pc, instructions);
            if (reports == MAX_REPORTS) {
                logalways("Reached %d RSP divergence reports, suppressing the rest", MAX_REPORTS);
            }
        }

        if (fatal_on_diff()) {
            logfatal("RSP dynarec diverged from the interpreter at 0x%03X", block_pc << 2);
        }
    }

    return instructions;
}
