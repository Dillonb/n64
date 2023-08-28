#include "dynarec.h"

#include <mem/n64bus.h>
#include <dynasm/dasm_proto.h>
#include <metrics.h>
#include "dynarec_memory_management.h"
#include "v1/v1_compiler.h"
#include "v2/v2_compiler.h"

// Uncomment to try to find idle loops
//#define DO_REPEATED_EXEC_DETECTION

n64_dynarec_t n64dynarec;

void update_sysconfig() {
    // handled by cp0_status_updated
    //n64dynarec.sysconfig.fr = N64CP0.status.fr;
}

int missing_block_handler(u32 physical_address, n64_dynarec_block_t* block, n64_block_sysconfig_t current_sysconfig) {
    u32 outer_index = physical_address >> BLOCKCACHE_OUTER_SHIFT;

    block->run = NULL;
    block->host_size = 0;
    block->guest_size = 0;
    block->next = NULL;
    block->sysconfig = current_sysconfig;
    block->virtual_address = N64CPU.pc;

    bool* code_mask = n64dynarec.code_mask[outer_index];

#ifdef N64_LOG_COMPILATIONS
    printf("Compilin' new block at 0x%08" PRIX64 " / 0x%08" PRIX32 "\n", N64CPU.pc, physical_address);
#endif

    mark_metric(METRIC_BLOCK_COMPILATION);
    v2_compile_new_block(block, code_mask, N64CPU.pc, physical_address);
    if (block->run == NULL) {
        logfatal("Failed to compile block!");
        //v1_compile_new_block(block, code_mask, N64CPU.pc, physical);
    }

    return n64dynarec.run_block((u64)block->run);
}

INLINE n64_dynarec_block_t* find_matching_block(n64_dynarec_block_t* blocks, n64_block_sysconfig_t current_sysconfig, u64 virtual_address) {
    n64_dynarec_block_t* block_iter = blocks;
    while (block_iter->run != NULL) {
        // make sure it matches the sysconfig and virtual address. If not, keep looking.
        if (block_iter->sysconfig.raw == current_sysconfig.raw && block_iter->virtual_address == virtual_address) {
            if (block_iter != blocks) {
                n64_dynarec_block_t temp = *blocks;
                copy_dynarec_block(blocks, block_iter);
                copy_dynarec_block(block_iter, &temp);
                return blocks;
            }
            return block_iter;
        } else {
            mark_metric(METRIC_BLOCK_SYSCONFIG_MISS); // block was valid, but did not match the current sysconfig.
        }
        // Add a block to the end of the list
        if (block_iter->next == NULL) {
            block_iter->next = dynarec_bumpalloc_zero(sizeof(n64_dynarec_block_t));
            return block_iter->next;
        }
        block_iter = block_iter->next;
    }
    return block_iter;
}

#ifdef DO_REPEATED_EXEC_DETECTION
#include <disassemble.h>
void do_repeated_exec_detection(u32 physical, n64_dynarec_block_t *block) {
    static u32 last_physical_address = 0;
    static size_t last_guest_size = 0;
    static int num_times_executed = 0;

    if (physical == last_physical_address) {
        num_times_executed++;
    } else {
        if (num_times_executed > 100) {
            if (last_physical_address < EREGION_RDRAM) {
                printf("Executed a block at %08X %d times\n", last_physical_address, num_times_executed);
                print_multi_guest(last_physical_address, &n64sys.mem.rdram[last_physical_address], last_guest_size);
            }
        }
        num_times_executed = 0;
    }
    last_physical_address = physical;
    last_guest_size = block->guest_size;
}
#endif // DO_REPEATED_EXEC_DETECTION

int n64_dynarec_step() {
    N64CPU.branch = false;
    N64CPU.prev_branch = false;
    u32 physical;
    if (!resolve_virtual_address(N64CPU.pc, BUS_LOAD, &physical)) {
        u64 fault_pc = N64CPU.pc;
        on_tlb_exception(fault_pc);
        r4300i_handle_exception(fault_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
        return 1; // TODO does exception handling have a cost by itself? does it matter?
    }

    u32 outer_index = physical >> BLOCKCACHE_OUTER_SHIFT;
    n64_dynarec_block_t* block_list = n64dynarec.blockcache[outer_index];

    if (unlikely(block_list == NULL)) {
#ifdef N64_LOG_COMPILATIONS
        printf("Need a new block list for page 0x%05X (address 0x%08X virtual 0x%08X)\n", outer_index, physical, N64CPU.pc);
#endif
        block_list = dynarec_bumpalloc_zero(BLOCKCACHE_INNER_SIZE * sizeof(n64_dynarec_block_t));
        for (int i = 0; i < BLOCKCACHE_INNER_SIZE; i++) {
            block_list[i].run = NULL;
            block_list[i].next = NULL;
            block_list[i].host_size = 0;
            block_list[i].guest_size = 0;
            block_list[i].sysconfig.raw = 0;
        }
        n64dynarec.blockcache[outer_index] = block_list;
        n64dynarec.code_mask[outer_index] = dynarec_bumpalloc_zero(BLOCKCACHE_INNER_SIZE * sizeof(bool));
    }

    u32 inner_index = BLOCKCACHE_INNER_INDEX(physical);
    n64_dynarec_block_t* block = &block_list[inner_index];


#ifdef LOG_ENABLED
    static long total_blocks_run;
    logdebug("Running block at 0x%016" PRIX64 " - block run #%ld - block FP: 0x%016" PRIX64, N64CPU.pc, ++total_blocks_run, (uintptr_t)block->run);
#endif
    N64CPU.exception = false;
    int taken;

    // Find the first block that's both non-null and matches the current sysconfig
    {
        n64_dynarec_block_t* matching_block = find_matching_block(block, n64dynarec.sysconfig, N64CPU.pc);
        if (matching_block && matching_block->run) {
            #ifdef DO_REPEATED_EXEC_DETECTION
            do_repeated_exec_detection(physical, block);
            #endif
            taken = n64dynarec.run_block((u64)matching_block->run);
        } else {
            return missing_block_handler(physical, matching_block, n64dynarec.sysconfig);
        }
    }
#ifdef N64_LOG_JIT_SYNC_POINTS
    printf("JITSYNC %d %08X ", taken, N64CPU.pc);
    for (int i = 0; i < 32; i++) {
        printf("%016" PRIX64, N64CPU.gpr[i]);
        if (i != 31) {
            printf(" ");
        }
    }
    printf("\n");
#endif
    logdebug("Done running block - took %d cycles - pc is now 0x%016" PRIX64, taken, N64CPU.pc);

    return taken * CYCLES_PER_INSTR;
}

void n64_dynarec_init(u8* codecache, size_t codecache_size) {
#ifdef N64_LOG_COMPILATIONS
    printf("Trying to malloc %ld bytes\n", sizeof(n64_dynarec_t));
#endif
    memset(&n64dynarec, 0, sizeof(n64_dynarec_t));

    n64dynarec.codecache_size = codecache_size;
    n64dynarec.codecache_used = 0;

    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        n64dynarec.blockcache[i] = NULL;
    }

    n64dynarec.codecache = codecache;

#ifdef N64_DYNAREC_V1_ENABLED
    v1_compiler_init();
#endif
    v2_compiler_init();
}

void invalidate_dynarec_all_pages() {
    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        n64dynarec.blockcache[i] = NULL;
    }
}