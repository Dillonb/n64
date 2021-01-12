#include "dynarec.h"

#include <mem/n64bus.h>
#include "cpu/dynarec/asm_emitter.h"
#include "dynarec_memory_management.h"


int missing_block_handler(r4300i_t* cpu) {
    word physical = resolve_virtual_address(cpu->pc, &cpu->cp0);
    word outer_index = physical >> BLOCKCACHE_OUTER_SHIFT;
    // TODO: put the dynarec object inside the r4300i_t object to get rid of this need for global_system
    n64_dynarec_block_t* block_list = global_system->dynarec->blockcache[outer_index];
    word inner_index = (physical & (BLOCKCACHE_PAGE_SIZE - 1)) >> 2;

    n64_dynarec_block_t* block = &block_list[inner_index];

#ifdef N64_LOG_COMPILATIONS
    printf("Compilin' new block at 0x%08X / 0x%08X\n", global_system->cpu.pc, physical);
#endif

    compile_new_block(global_system->dynarec, cpu, block, cpu->pc, physical);

    return block->run(cpu);
}

int n64_dynarec_step(n64_system_t* system, n64_dynarec_t* dynarec) {
    word physical = resolve_virtual_address(system->cpu.pc, &system->cpu.cp0);
    word outer_index = physical >> BLOCKCACHE_OUTER_SHIFT;
    n64_dynarec_block_t* block_list = dynarec->blockcache[outer_index];
    word inner_index = (physical & (BLOCKCACHE_PAGE_SIZE - 1)) >> 2;

    if (unlikely(block_list == NULL)) {
#ifdef N64_LOG_COMPILATIONS
        printf("Need a new block list for page 0x%05X (address 0x%08X virtual 0x%08X)\n", outer_index, physical, system->cpu.pc);
#endif
        block_list = dynarec_bumpalloc_zero(dynarec, BLOCKCACHE_INNER_SIZE * sizeof(n64_dynarec_block_t));
        for (int i = 0; i < BLOCKCACHE_INNER_SIZE; i++) {
            block_list[i].run = missing_block_handler;
        }
        dynarec->blockcache[outer_index] = block_list;
    }

    n64_dynarec_block_t* block = &block_list[inner_index];

    static long total_blocks_run;
    logdebug("Running block at 0x%016lX - block run #%ld - block FP: 0x%016lX", system->cpu.pc, ++total_blocks_run, (uintptr_t)block->run);
    int taken = block->run(&system->cpu);
#ifdef N64_LOG_JIT_SYNC_POINTS
    printf("JITSYNC %d %08X ", taken, system->cpu.pc);
    for (int i = 0; i < 32; i++) {
        printf("%016lX", system->cpu.gpr[i]);
        if (i != 31) {
            printf(" ");
        }
    }
    printf("\n");
#endif
    logdebug("Done running block - took %d cycles - pc is now 0x%016lX", taken, system->cpu.pc);

    return taken * CYCLES_PER_INSTR;
}

n64_dynarec_t* n64_dynarec_init(n64_system_t* system, byte* codecache, size_t codecache_size) {
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
    return dynarec;
}

void invalidate_dynarec_page(n64_dynarec_t* dynarec, word physical_address) {
    word outer_index = physical_address >> BLOCKCACHE_OUTER_SHIFT;
    dynarec->blockcache[outer_index] = NULL;
}
