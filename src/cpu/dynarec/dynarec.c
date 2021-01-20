#include "dynarec.h"

#include <mem/n64bus.h>
#include <dynasm/dasm_proto.h>
#include <metrics.h>
#include "cpu/dynarec/asm_emitter.h"
#include "dynarec_memory_management.h"

#define IS_PAGE_BOUNDARY(address) ((address & (BLOCKCACHE_PAGE_SIZE - 1)) == 0)

void* link_and_encode(n64_dynarec_t* dynarec, dasm_State** d) {
    size_t code_size;
    dasm_link(d, &code_size);
#ifdef N64_LOG_COMPILATIONS
    printf("Generated %ld bytes of code\n", code_size);
#endif
    void* buf = dynarec_bumpalloc(dynarec, code_size);
    dasm_encode(d, buf);

    return buf;
}

INLINE bool is_branch(dynarec_instruction_category_t category) {
    return category == BRANCH || category == BRANCH_LIKELY;
}

static int arg_host_registers[] = {0, 0};
static int dest_host_register = 0;
static int valid_host_regs[32];
static int num_valid_host_regs;
static int num_available_host_regs;
static bool guest_reg_loaded[32];
static bool host_reg_used[32];
static int guest_reg_to_host_reg[32];

INLINE bool is_reg_loaded(int guest) {
    return guest_reg_loaded[guest];
}

INLINE bool is_reg_used(int host) {
    return host_reg_used[host];
}

INLINE int get_valid_host_reg() {
    for (int r = 0; r < num_valid_host_regs; r++) {
        if (!is_reg_used(r)) {
            return r;
        }
    }

    logfatal("Ran out of valid host regs! Should have flushed!");
}

INLINE void flush_reg(dasm_State** Dst, r4300i_t* cpu, int guest) {
    if (is_reg_loaded(guest)) {
        int host_reg = guest_reg_to_host_reg[guest];
        flush_host_register_to_gpr(Dst, cpu, valid_host_regs[host_reg], guest);
        num_available_host_regs++;
    }
    guest_reg_loaded[guest] = false;
    host_reg_used[guest_reg_to_host_reg[guest]] = false;
}

INLINE int load_reg(dasm_State** Dst, r4300i_t* cpu, int guest) {
    if (!is_reg_loaded(guest)) {
        guest_reg_loaded[guest] = true;
        int host_reg = get_valid_host_reg();
        num_available_host_regs--;

        guest_reg_to_host_reg[guest] = host_reg;
        host_reg_used[host_reg] = true;

        load_host_register_from_gpr(Dst, cpu, valid_host_regs[host_reg], guest);
    }
    int valid_host_reg = valid_host_regs[guest_reg_to_host_reg[guest]];
    return valid_host_reg;
}

INLINE void flush_all(dasm_State** Dst, r4300i_t* cpu) {
    for (int r = 0; r < 32; r++) {
        if (is_reg_loaded(r)) {
            flush_reg(Dst, cpu, r);
        }
    }
}

INLINE void load_reg_1(dasm_State** Dst, r4300i_t* cpu, int* dest1, int r1) {
    bool r1_loaded = is_reg_loaded(r1);

    if (num_available_host_regs >= 1 || r1_loaded) {
        *dest1 = load_reg(Dst, cpu, r1);
    } else {
        flush_all(Dst, cpu);
        *dest1 = load_reg(Dst, cpu, r1);
    }
}

INLINE void load_reg_2(dasm_State** Dst, r4300i_t* cpu, int* dest1, int r1, int* dest2, int r2) {
    bool r1_loaded = is_reg_loaded(r1);
    bool r2_loaded = is_reg_loaded(r2);

    if (num_available_host_regs >= 2 || (r1_loaded && r2_loaded)) {
        *dest1 = load_reg(Dst, cpu, r1);
        *dest2 = load_reg(Dst, cpu, r2);
    } else {
        flush_all(Dst, cpu);
        *dest1 = load_reg(Dst, cpu, r1);
        *dest2 = load_reg(Dst, cpu, r2);
    }
}

INLINE void load_reg_3(dasm_State** Dst, r4300i_t* cpu, int* dest1, int r1, int* dest2, int r2, int* dest3, int r3) {
    bool r1_loaded = is_reg_loaded(r1);
    bool r2_loaded = is_reg_loaded(r2);
    bool r3_loaded = is_reg_loaded(r3);

    if (num_available_host_regs >= 3 || (r1_loaded && r2_loaded && r3_loaded)) {
        *dest1 = load_reg(Dst, cpu, r1);
        *dest2 = load_reg(Dst, cpu, r2);
        *dest3 = load_reg(Dst, cpu, r3);
    } else {
        flush_all(Dst, cpu);
        *dest1 = load_reg(Dst, cpu, r1);
        *dest2 = load_reg(Dst, cpu, r2);
        *dest3 = load_reg(Dst, cpu, r3);
    }
}

void compile_new_block(n64_dynarec_t* dynarec, r4300i_t* compile_time_cpu, n64_dynarec_block_t* block, dword virtual_address, word physical_address) {
    mark_metric(METRIC_BLOCK_COMPILATION);
    static dasm_State* d;
    d = block_header();
    dasm_State** Dst = &d;

    memset(guest_reg_loaded, 0, sizeof(guest_reg_loaded));
    memset(host_reg_used, 0, sizeof(host_reg_used));

    num_available_host_regs = num_valid_host_regs;

    bool should_continue_block = true;
    int block_length = 0;


    int instructions_left_in_block = -1;

    dynarec_instruction_category_t prev_instr_category = NORMAL;

    bool branch_in_block = false;

    do {
        mips_instruction_t instr;
        instr.raw = n64_read_physical_word(physical_address);

        word next_physical_address = physical_address + 4;
        dword next_virtual_address = virtual_address + 4;

        instructions_left_in_block--;
        bool instr_ends_block;

        word extra_cycles = 0;
        dynarec_ir_t* ir = instruction_ir(instr, physical_address);
        if (ir->exception_possible) {
            // save prev_pc
            // TODO will no longer need this when we emit code to check the exceptions
            flush_prev_pc(Dst, virtual_address);
        }
        if (is_branch(ir->category)) {
            flush_pc(Dst, next_virtual_address);
            flush_next_pc(Dst, next_virtual_address + 4);
            clear_branch_flag(Dst);
        }
        switch (ir->format) {
            case CALL_INTERPRETER:
                flush_all(Dst, compile_time_cpu);
                break;
            case FORMAT_NOP:break; // Shouldn't touch any registers, so no need to do anything
            case SHIFT_CONST:
                load_reg_2(Dst, compile_time_cpu, &arg_host_registers[0], instr.r.rt, &dest_host_register, instr.r.rd);
                break;
            case I_TYPE:
                load_reg_2(Dst, compile_time_cpu, &arg_host_registers[0], instr.i.rs, &dest_host_register, instr.i.rt);
                break;
            case R_TYPE:
                load_reg_3(Dst, compile_time_cpu, &arg_host_registers[0], instr.r.rt, &arg_host_registers[1], instr.r.rs, &dest_host_register, instr.r.rd);
                break;
            case J_TYPE:
                logfatal("Allocate regs for J_TYPE");
                break;
            case MF_MULTREG:
                load_reg_1(Dst, compile_time_cpu, &dest_host_register, instr.r.rd);
                break;
            case MT_MULTREG:
                load_reg_1(Dst, compile_time_cpu, &arg_host_registers[0], instr.r.rs);
                break;
        }
        ir->compiler(Dst, instr, physical_address, arg_host_registers, dest_host_register, &extra_cycles);
        block_length++;
        block_length += extra_cycles;
        if (ir->exception_possible) {
            check_exception(Dst, block_length);
        }

        switch (ir->category) {
            case NORMAL:
                instr_ends_block = instructions_left_in_block == 0;
                break;
            case BRANCH:
                branch_in_block = true;
                advance_pc(compile_time_cpu, Dst);
                if (prev_instr_category == BRANCH || prev_instr_category == BRANCH_LIKELY) {
                    // Check if the previous branch was taken.

                    // If the last branch wasn't taken, we can treat this the same as if the previous instruction wasn't a branch
                    // just set the cpu->last_branch_taken to cpu->branch_taken and execute the next instruction.

                    // emit:
                    // if (!cpu->last_branch_taken) cpu->last_branch_taken = cpu->branch_taken;
                    logfatal("Branch in a branch delay slot");
                } else {
                    // If the last instruction wasn't a branch, no special behavior is needed. Just set up some state in case the next one is.
                    // emit:
                    // cpu->last_branch_taken = cpu->branch_taken;
                    //logfatal("unimp");
                }

                instr_ends_block = false;
                instructions_left_in_block = 1; // emit delay slot
                break;

            case BRANCH_LIKELY:
                branch_in_block = true;
                // If the previous instruction was a branch:
                if (prev_instr_category == BRANCH || prev_instr_category == BRANCH_LIKELY) {
                    logfatal("Branch in a branch likely delay slot");
                } else {
                    // TODO: only need to flush all if we exit the block early
                    flush_all(Dst, compile_time_cpu);
                    post_branch_likely(Dst, compile_time_cpu, block_length);
                }

                instr_ends_block = false;
                instructions_left_in_block = 1; // emit delay slot
                break;

            case ERET:
                branch_in_block = true;
                instr_ends_block = true;
                break;

            case TLB_WRITE:
            case STORE:
                instr_ends_block = true;
                break;

            default:
                logfatal("Unknown dynarec instruction type");
        }

        bool page_boundary_ends_block = IS_PAGE_BOUNDARY(next_physical_address);
        // !!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!
        // If the first instruction in the new page is a delay slot, INCLUDE IT IN THE BLOCK ANYWAY.
        // This DOES BREAK a corner case!
        // If the game overwrites the delay slot but does not overwrite the branch or anything in the other page,
        // THIS BLOCK WILL NOT GET MARKED DIRTY.
        // I highly doubt any games do it, but THIS NEEDS TO GET FIXED AT SOME POINT
        // !!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!
        if (instructions_left_in_block == 1) { page_boundary_ends_block = false; } // FIXME, TODO, BAD, EVIL, etc

        if (instr_ends_block || page_boundary_ends_block) {
#ifdef N64_LOG_COMPILATIONS
            printf("Ending block. instr: %d pb: %d (0x%08X)\n", instr_ends_block, page_boundary_ends_block, next_physical_address);
#endif
            if (!branch_in_block) {
                flush_pc(Dst, next_virtual_address);
                flush_next_pc(Dst, next_virtual_address + 4);
            }
            should_continue_block = false;
        }

        physical_address = next_physical_address;
        virtual_address = next_virtual_address;
        prev_instr_category = ir->category;
    } while (should_continue_block);
    flush_all(Dst, compile_time_cpu);
    end_block(Dst, block_length);
    void* compiled = link_and_encode(dynarec, &d);
    dasm_free(&d);

    block->run = compiled;
}


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

#ifdef LOG_ENABLED
    static long total_blocks_run;
    logdebug("Running block at 0x%016lX - block run #%ld - block FP: 0x%016lX", system->cpu.pc, ++total_blocks_run, (uintptr_t)block->run);
#endif
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

    num_valid_host_regs = 32;
    fill_valid_host_regs(valid_host_regs, &num_valid_host_regs);

    return dynarec;
}

void invalidate_dynarec_page(n64_dynarec_t* dynarec, word physical_address) {
    word outer_index = physical_address >> BLOCKCACHE_OUTER_SHIFT;
    dynarec->blockcache[outer_index] = NULL;
}

void invalidate_dynarec_all_pages(n64_dynarec_t* dynarec) {
    for (int i = 0; i < BLOCKCACHE_OUTER_SIZE; i++) {
        dynarec->blockcache[i] = NULL;
    }
}