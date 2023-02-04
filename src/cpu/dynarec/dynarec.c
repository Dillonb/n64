#include "dynarec.h"

#include <mem/n64bus.h>
#include <dynasm/dasm_proto.h>
#include <metrics.h>
#include <dynarec/v2/dispatcher.h>
#include "dynarec_memory_management.h"
#include "v1/v1_compiler.h"
#include "v2/v2_compiler.h"

static int missing_block_handler() {
    u32 physical = resolve_virtual_address_or_die(N64CPU.pc, BUS_LOAD);
    u32 outer_index = physical >> BLOCKCACHE_OUTER_SHIFT;
    n64_dynarec_block_t* block_list = N64DYNAREC->blockcache[outer_index];
    u32 inner_index = (physical & (BLOCKCACHE_PAGE_SIZE - 1)) >> 2;

    n64_dynarec_block_t* block = &block_list[inner_index];
    block->run = NULL;
    bool* code_mask = N64DYNAREC->code_mask[outer_index];

#ifdef N64_LOG_COMPILATIONS
    printf("Compilin' new block at 0x%08X / 0x%08X\n", N64CPU.pc, physical);
#endif

    mark_metric(METRIC_BLOCK_COMPILATION);
    v2_compile_new_block(block, code_mask, N64CPU.pc, physical);
    if (block->run == NULL) {
        logfatal("Failed to compile block!");
        //v1_compile_new_block(block, code_mask, N64CPU.pc, physical);
    }

    return run_block((u64)block->run);
}

int n64_dynarec_step() {
    u32 physical;
    if (!resolve_virtual_address(N64CPU.pc, BUS_LOAD, &physical)) {
        on_tlb_exception(N64CPU.pc);
        r4300i_handle_exception(N64CPU.pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
        printf("TLB miss PC, now at %016lX\n", N64CPU.pc);
        return 1; // TODO does exception handling have a cost by itself? does it matter?
    }

    u32 outer_index = physical >> BLOCKCACHE_OUTER_SHIFT;
    n64_dynarec_block_t* block_list = N64DYNAREC->blockcache[outer_index];
    u32 inner_index = BLOCKCACHE_INNER_INDEX(physical);

    if (unlikely(block_list == NULL)) {
#ifdef N64_LOG_COMPILATIONS
        printf("Need a new block list for page 0x%05X (address 0x%08X virtual 0x%08X)\n", outer_index, physical, N64CPU.pc);
#endif
        block_list = dynarec_bumpalloc_zero(BLOCKCACHE_INNER_SIZE * sizeof(n64_dynarec_block_t));
        for (int i = 0; i < BLOCKCACHE_INNER_SIZE; i++) {
            block_list[i].run = missing_block_handler;
        }
        N64DYNAREC->blockcache[outer_index] = block_list;
        N64DYNAREC->code_mask[outer_index] = dynarec_bumpalloc_zero(BLOCKCACHE_INNER_SIZE * sizeof(bool));
    }

    n64_dynarec_block_t* block = &block_list[inner_index];

#ifdef LOG_ENABLED
    static long total_blocks_run;
    logdebug("Running block at 0x%016lX - block run #%ld - block FP: 0x%016lX", N64CPU.pc, ++total_blocks_run, (uintptr_t)block->run);
#endif
    N64CPU.exception = false;
    int taken = block->run(&N64CPU);
#ifdef N64_LOG_JIT_SYNC_POINTS
    printf("JITSYNC %d %08X ", taken, N64CPU.pc);
    for (int i = 0; i < 32; i++) {
        printf("%016lX", N64CPU.gpr[i]);
        if (i != 31) {
            printf(" ");
        }
    }
    printf("\n");
#endif
    logdebug("Done running block - took %d cycles - pc is now 0x%016lX", taken, N64CPU.pc);

    return taken * CYCLES_PER_INSTR;
}

n64_dynarec_t* n64_dynarec_init(u8* codecache, size_t codecache_size) {
#ifdef N64_LOG_COMPILATIONS
    printf("Trying to malloc %ld bytes\n", sizeof(n64_dynarec_t));
#endif
    n64_dynarec_t* dynarec = calloc(1, sizeof(n64_dynarec_t));

    dynarec->codecache_size = codecache_size;
    dynarec->codecache_used = 0;

    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        dynarec->blockcache[i] = NULL;
    }

    dynarec->codecache = codecache;

    v1_compiler_init();
    v2_compiler_init();

    return dynarec;
}

void invalidate_dynarec_all_pages(n64_dynarec_t* dynarec) {
    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        dynarec->blockcache[i] = NULL;
    }
}