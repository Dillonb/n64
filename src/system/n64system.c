#include "n64system.h"

#include <string.h>
#include <unistd.h>

#include <mem/n64bus.h>
#include <frontend/render.h>
#include <interface/vi.h>
#include <interface/ai.h>
#include <n64_rsp_bus.h>
#include <cpu/rsp.h>
#include <cpu/dynarec/dynarec.h>
#include <sys/mman.h>
#include <errno.h>
#include <mem/backup.h>
#include <frontend/game_db.h>
#include <metrics.h>

static bool should_quit = false;


n64_system_t* global_system;

// 128MiB codecache
#define CODECACHE_SIZE (1 << 27)
static byte codecache[CODECACHE_SIZE] __attribute__((aligned(4096)));

bool n64_should_quit() {
    return should_quit;
}

void write_physical_word_wrapper(word address, word value) {
    n64_write_word(global_system, address, value);
}

dword virtual_read_dword_wrapper(dword address) {
    word physical = resolve_virtual_address(address, &global_system->cpu.cp0);
    return n64_read_dword(global_system, physical);
}

void virtual_write_dword_wrapper(dword address, dword value) {
    word physical = resolve_virtual_address(address, &global_system->cpu.cp0);
    n64_write_dword(global_system, physical, value);
}

word virtual_read_word_wrapper(dword address) {
    word physical = resolve_virtual_address(address, &global_system->cpu.cp0);
    return n64_read_physical_word(physical);
}

void virtual_write_word_wrapper(dword address, word value) {
    word physical = resolve_virtual_address(address, &global_system->cpu.cp0);
    n64_write_word(global_system, physical, value);
}

half virtual_read_half_wrapper(dword address) {
    word physical = resolve_virtual_address(address, &global_system->cpu.cp0);
    return n64_read_half(global_system, physical);
}

void virtual_write_half_wrapper(dword address, half value) {
    word physical = resolve_virtual_address(address, &global_system->cpu.cp0);
    n64_write_half(global_system, physical, value);
}

byte virtual_read_byte_wrapper(dword address) {
    word physical = resolve_virtual_address(address, &global_system->cpu.cp0);
    return n64_read_byte(global_system, physical);
}

void virtual_write_byte_wrapper(dword address, byte value) {
    word physical = resolve_virtual_address(address, &global_system->cpu.cp0);
    n64_write_byte(global_system, physical, value);
}

void n64_load_rom(n64_system_t* system, const char* rom_path) {
    logalways("Loading %s", rom_path);
    load_n64rom(&system->mem.rom, rom_path);
    gamedb_match(system);
    init_savedata(&system->mem, rom_path);
    strcpy(system->rom_path, rom_path);
}

n64_system_t* init_n64system(const char* rom_path, bool enable_frontend, bool enable_debug, n64_video_type_t video_type, bool use_interpreter) {
    // align to page boundary
    n64_system_t* system;
    posix_memalign((void **) &system, sysconf(_SC_PAGESIZE), sizeof(n64_system_t));

    memset(system, 0x00, sizeof(n64_system_t));
    init_mem(&system->mem);

    system->video_type = video_type;

    system->cpu.read_dword = &virtual_read_dword_wrapper;
    system->cpu.write_dword = &virtual_write_dword_wrapper;

    system->cpu.read_word = &virtual_read_word_wrapper;
    system->cpu.write_word = &virtual_write_word_wrapper;

    system->cpu.read_half = &virtual_read_half_wrapper;
    system->cpu.write_half = &virtual_write_half_wrapper;

    system->cpu.read_byte = &virtual_read_byte_wrapper;
    system->cpu.write_byte = &virtual_write_byte_wrapper;

    system->cpu.resolve_virtual_address = &resolve_virtual_address;

    system->rsp.read_physical_word = &n64_read_physical_word;
    system->rsp.write_physical_word = &write_physical_word_wrapper;

    if (mprotect(&codecache, CODECACHE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        logfatal("mprotect codecache failed! %s", strerror(errno));
    }
    system->dynarec = n64_dynarec_init(system, codecache, CODECACHE_SIZE);

    global_system = system;
    if (enable_frontend) {
        render_init(system, video_type);
    }
    system->debugger_state.enabled = enable_debug;
    if (enable_debug) {
        debugger_init(system);
    }
    system->use_interpreter = use_interpreter;

    reset_n64system(system);

    if (rom_path != NULL) {
        n64_load_rom(system, rom_path);
    }

    return system;
}

void reset_n64system(n64_system_t* system) {
    force_persist_backup(system);
    if (system->mem.save_data != NULL) {
        free(system->mem.save_data);
        system->mem.save_data = NULL;
    }
    if (system->mem.mempack_data != NULL) {
        free(system->mem.mempack_data);
        system->mem.mempack_data = NULL;
    }
    system->cpu.branch = false;
    system->cpu.exception = false;


    for (int i = 0; i < SP_IMEM_SIZE / 4; i++) {
        system->rsp.icache[i].instruction.raw = 0;
        system->rsp.icache[i].handler = cache_rsp_instruction;
    }

    system->rsp.status.halt = true; // RSP starts halted

    system->vi.vi_v_intr = 256;

    system->dpc.status.raw = 0x80;


    system->ai.dac.frequency = 44100;
    system->ai.dac.precision = 16;
    system->ai.dac.period = CPU_HERTZ / system->ai.dac.frequency;

    system->si.controllers[0].plugged_in = true;
    system->si.controllers[1].plugged_in = false;
    system->si.controllers[2].plugged_in = false;
    system->si.controllers[3].plugged_in = false;

    system->cpu.cp0.status.bev = true;
    system->cpu.cp0.cause.raw  = 0xB000007C;
    system->cpu.cp0.EPC        = 0xFFFFFFFFFFFFFFFF;
    system->cpu.cp0.PRId       = 0x00000B22;
    system->cpu.cp0.config     = 0x70000000;
    system->cpu.cp0.error_epc  = 0xFFFFFFFFFFFFFFFF;

    memset(system->mem.rdram, 0, N64_RDRAM_SIZE);
    memset(system->rsp.sp_dmem, 0, SP_DMEM_SIZE);
    memset(system->rsp.sp_imem, 0, SP_IMEM_SIZE);
    memset(system->mem.pif_ram, 0, PIF_RAM_SIZE);

    invalidate_dynarec_all_pages(system->dynarec);
}

INLINE int jit_system_step(n64_system_t* system) {
    r4300i_t* cpu = &system->cpu;

    /* Commented out for now since the game never actually reads cp0.random
     * TODO: when a game does, consider generating a random number rather than updating this every instruction
    if (cpu->cp0.random <= cpu->cp0.wired) {
        cpu->cp0.random = 31;
    } else {
        cpu->cp0.random--;
    }
     */

    if (unlikely(cpu->interrupts > 0)) {
        if(cpu->cp0.status.ie && !cpu->cp0.status.exl && !cpu->cp0.status.erl) {
            r4300i_handle_exception(cpu, cpu->pc, EXCEPTION_INTERRUPT, -1);
            return CYCLES_PER_INSTR;
        }
    }
    static int cpu_steps = 0;
    int taken = n64_dynarec_step(system, system->dynarec);
    {
        uint64_t oldcount = cpu->cp0.count >> 1;
        uint64_t newcount = (cpu->cp0.count + (taken * CYCLES_PER_INSTR)) >> 1;
        if (unlikely(oldcount < cpu->cp0.compare && newcount >= cpu->cp0.compare)) {
            cpu->cp0.cause.ip7 = true;
            loginfo("Compare interrupt! oldcount: 0x%08lX newcount: 0x%08lX compare 0x%08X", oldcount, newcount, cpu->cp0.compare);
            r4300i_interrupt_update(cpu);
        }
        cpu->cp0.count += taken;
        cpu->cp0.count &= 0x1FFFFFFFF;
    }
    cpu_steps += taken;

    if (!system->rsp.status.halt) {
        // 2 RSP steps per 3 CPU steps
        while (cpu_steps > 2) {
            system->rsp.steps += 2;
            cpu_steps -= 3;
        }

        rsp_run(&system->rsp);
    } else {
        system->rsp.steps = 0;
        cpu_steps = 0;
    }

    return taken;
}

INLINE int interpreter_system_step(n64_system_t* system) {
#ifdef N64_DEBUG_MODE
    if (system->debugger_state.enabled && check_breakpoint(&system->debugger_state, system->cpu.pc)) {
        debugger_breakpoint_hit(system);
    }
    while (system->debugger_state.broken) {
        usleep(1000);
        debugger_tick(system);
    }
#endif
    int taken = CYCLES_PER_INSTR;
    r4300i_step(&system->cpu);
    static int cpu_steps = 0;
    cpu_steps += taken;

    if (system->rsp.status.halt) {
        cpu_steps = 0;
        system->rsp.steps = 0;
    } else {
        // 2 RSP steps per 3 CPU steps
        while (cpu_steps > 2) {
            system->rsp.steps += 2;
            cpu_steps -= 3;
        }
        rsp_run(&system->rsp);
    }

    return taken;
}

// This is used for debugging tools, it's fine for now if timing is a little off.
void n64_system_step(n64_system_t* system, bool dynarec) {
    if (dynarec) {
        jit_system_step(system);
    } else {
        r4300i_step(&system->cpu);
        if (!system->rsp.status.halt) {
            rsp_step(&system->rsp);
        }
    }
}

void check_vsync(n64_system_t* system) {
    if (system->vi.v_current == system->vi.vsync >> 1) {
        rdp_update_screen(system);
    }
}

void jit_system_loop(n64_system_t* system) {
    int cycles = 0;
    while (!should_quit) {
        for (system->vi.v_current = 0; system->vi.v_current < NUM_SHORTLINES; system->vi.v_current++) {
            check_vi_interrupt(system);
            check_vsync(system);
            while (cycles <= SHORTLINE_CYCLES) {
                cycles += jit_system_step(system);
                system->debugger_state.steps = 0;
            }
            cycles -= SHORTLINE_CYCLES;
            ai_step(system, SHORTLINE_CYCLES);
        }
        for (; system->vi.v_current < NUM_SHORTLINES + NUM_LONGLINES; system->vi.v_current++) {
            check_vi_interrupt(system);
            check_vsync(system);
            while (cycles <= LONGLINE_CYCLES) {
                cycles += jit_system_step(system);
                system->debugger_state.steps = 0;
            }
            cycles -= LONGLINE_CYCLES;
            ai_step(system, LONGLINE_CYCLES);
        }
        check_vi_interrupt(system);
        check_vsync(system);
#ifdef N64_DEBUG_MODE
        if (system->debugger_state.enabled) {
            debugger_tick(system);
        }
#endif
#ifdef LOG_ENABLED
update_delayed_log_verbosity();
#endif
        persist_backup(system);
        reset_all_metrics();
    }
    force_persist_backup(system);
}

void interpreter_system_loop(n64_system_t* system) {
    int cycles = 0;
    while (!should_quit) {
        for (system->vi.v_current = 0; system->vi.v_current < NUM_SHORTLINES; system->vi.v_current++) {
            check_vi_interrupt(system);
            check_vsync(system);
            while (cycles <= SHORTLINE_CYCLES) {
                cycles += interpreter_system_step(system);
                system->debugger_state.steps = 0;
            }
            cycles -= SHORTLINE_CYCLES;
            ai_step(system, SHORTLINE_CYCLES);
        }
        for (; system->vi.v_current < NUM_SHORTLINES + NUM_LONGLINES; system->vi.v_current++) {
            check_vi_interrupt(system);
            check_vsync(system);
            while (cycles <= LONGLINE_CYCLES) {
                cycles += interpreter_system_step(system);
                system->debugger_state.steps = 0;
            }
            cycles -= LONGLINE_CYCLES;
            ai_step(system, LONGLINE_CYCLES);
        }
        check_vi_interrupt(system);
        check_vsync(system);
#ifdef N64_DEBUG_MODE
        if (system->debugger_state.enabled) {
            debugger_tick(system);
        }
#endif
#ifdef LOG_ENABLED
        update_delayed_log_verbosity();
#endif
        persist_backup(system);
        reset_all_metrics();
    }
    force_persist_backup(system);
}

void n64_system_loop(n64_system_t* system) {
    if (system->use_interpreter) {
        interpreter_system_loop(system);
    } else {
        jit_system_loop(system);
    }
}

void n64_system_cleanup(n64_system_t* system) {
    rdp_cleanup();
    if (system->dynarec != NULL) {
        free(system->dynarec);
        system->dynarec = NULL;
    }
    debugger_cleanup(system);

    free(system->mem.rom.rom);
    system->mem.rom.rom = NULL;

    free(system->mem.rom.pif_rom);
    system->mem.rom.pif_rom = NULL;

    free(system);
}

void n64_request_quit() {
    should_quit = true;
}

void on_interrupt_change(n64_system_t* system) {
    bool interrupt = system->mi.intr.raw & system->mi.intr_mask.raw;
    loginfo("ip2 is now: %d", interrupt);
    system->cpu.cp0.cause.ip2 = interrupt;
    r4300i_interrupt_update(&system->cpu);
}

void interrupt_raise(n64_interrupt_t interrupt) {
    switch (interrupt) {
        case INTERRUPT_VI:
            loginfo("Raising VI interrupt");
            global_system->mi.intr.vi = true;
            break;
        case INTERRUPT_SI:
            loginfo("Raising SI interrupt");
            global_system->mi.intr.si = true;
            break;
        case INTERRUPT_PI:
            loginfo("Raising PI interrupt");
            global_system->mi.intr.pi = true;
            break;
        case INTERRUPT_AI:
            loginfo("Raising AI interrupt");
            global_system->mi.intr.ai = true;
            break;
        case INTERRUPT_DP:
            loginfo("Raising DP interrupt");
            global_system->mi.intr.dp = true;
            break;
        case INTERRUPT_SP:
            loginfo("Raising SP interrupt");
            global_system->mi.intr.sp = true;
            break;
        default:
            logfatal("Raising unimplemented interrupt: %d", interrupt);
    }

    on_interrupt_change(global_system);
}

void interrupt_lower(n64_system_t* system, n64_interrupt_t interrupt) {
    switch (interrupt) {
        case INTERRUPT_VI:
            system->mi.intr.vi = false;
            loginfo("Lowering VI interrupt");
            break;
        case INTERRUPT_SI:
            system->mi.intr.si = false;
            loginfo("Lowering SI interrupt");
            break;
        case INTERRUPT_PI:
            system->mi.intr.pi = false;
            loginfo("Lowering PI interrupt");
            break;
        case INTERRUPT_DP:
            system->mi.intr.dp = false;
            loginfo("Lowering DP interrupt");
            break;
        case INTERRUPT_AI:
            system->mi.intr.ai = false;
            loginfo("Lowering DP interrupt");
            break;
        case INTERRUPT_SP:
            system->mi.intr.sp = false;
            loginfo("Lowering SP interrupt");
            break;
        default:
            logfatal("Lowering unimplemented interrupt: %d", interrupt);
    }

    on_interrupt_change(system);
}
