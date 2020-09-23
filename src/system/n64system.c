#include "n64system.h"

#include <string.h>
#include <gdbstub.h>
#include <unistd.h>
#include <time.h>

#include <mem/n64bus.h>
#include <frontend/render.h>
#include <interface/vi.h>
#include <interface/ai.h>
#include <mem/n64_rsp_bus.h>
#include <cpu/rsp.h>

// The CPU runs at 93.75mhz. There are 60 frames per second, and 262 lines on the display.
// There are 1562500 cycles per frame.
// Because this doesn't divide nicely by 262, we have to run some lines for 1 more cycle than others.
// We call these the "long" lines, and the others the "short" lines.

// 5963*68+5964*194 == 1562500

#define NUM_SHORTLINES 68
#define NUM_LONGLINES  194

#define SHORTLINE_CYCLES 5963
#define LONGLINE_CYCLES  5964

bool should_quit = false;


n64_system_t* global_system;

/* TODO I'm 99% sure the RSP can't read/write DWORDs
dword read_rsp_dword_wrapper(word address) {
    return n64_rsp_read_dword(global_system, address);
}

void write_rsp_dword_wrapper(word address, dword value) {
    n64_rsp_write_dword(global_system, address, value);
}
 */

word read_rsp_word_wrapper(word address) {
    return n64_rsp_read_word(global_system, address);
}

void write_rsp_word_wrapper(word address, word value) {
    n64_rsp_write_word(global_system, address, value);
}

word read_physical_word_wrapper(word address) {
    return n64_read_word(global_system, address);
}

void write_physical_word_wrapper(word address, word value) {
    n64_write_word(global_system, address, value);
}

half read_rsp_half_wrapper(word address) {
    return n64_rsp_read_half(global_system, address);
}

void write_rsp_half_wrapper(word address, half value) {
    n64_rsp_write_half(global_system, address, value);
}

byte read_rsp_byte_wrapper(word address) {
    return n64_rsp_read_byte(global_system, address);
}

void write_rsp_byte_wrapper(word address, byte value) {
    n64_rsp_write_byte(global_system, address, value);
}

byte read_physical_byte_wrapper(word address) {
    return n64_read_byte(global_system, address);
}

void write_physical_byte_wrapper(word address, byte value) {
    n64_write_byte(global_system, address, value);
}

dword virtual_read_dword_wrapper(word address) {
    address = resolve_virtual_address(address, &global_system->cpu.cp0);
    return n64_read_dword(global_system, address);
}

void virtual_write_dword_wrapper(word address, dword value) {
    address = resolve_virtual_address(address, &global_system->cpu.cp0);
    n64_write_dword(global_system, address, value);
}

word virtual_read_word_wrapper(word address) {
    address = resolve_virtual_address(address, &global_system->cpu.cp0);
    return n64_read_word(global_system, address);
}

void virtual_write_word_wrapper(word address, word value) {
    address = resolve_virtual_address(address, &global_system->cpu.cp0);
    n64_write_word(global_system, address, value);
}

half virtual_read_half_wrapper(word address) {
    address = resolve_virtual_address(address, &global_system->cpu.cp0);
    return n64_read_half(global_system, address);
}

void virtual_write_half_wrapper(word address, half value) {
    address = resolve_virtual_address(address, &global_system->cpu.cp0);
    n64_write_half(global_system, address, value);
}

byte virtual_read_byte_wrapper(word address) {
    address = resolve_virtual_address(address, &global_system->cpu.cp0);
    return n64_read_byte(global_system, address);
}

void virtual_write_byte_wrapper(word address, byte value) {
    address = resolve_virtual_address(address, &global_system->cpu.cp0);
    n64_write_byte(global_system, address, value);
}

n64_system_t* init_n64system(const char* rom_path, bool enable_frontend, bool enable_debug) {
    n64_system_t* system = malloc(sizeof(n64_system_t));
    memset(system, 0x00, sizeof(n64_system_t));
    init_mem(&system->mem);
    if (rom_path != NULL) {
        load_n64rom(&system->mem.rom, rom_path);
    }

    system->cpu.read_dword = &virtual_read_dword_wrapper;
    system->cpu.write_dword = &virtual_write_dword_wrapper;

    system->cpu.read_word = &virtual_read_word_wrapper;
    system->cpu.write_word = &virtual_write_word_wrapper;

    system->cpu.read_half = &virtual_read_half_wrapper;
    system->cpu.write_half = &virtual_write_half_wrapper;

    system->cpu.read_byte = &virtual_read_byte_wrapper;
    system->cpu.write_byte = &virtual_write_byte_wrapper;

    //system->rsp.read_dword = &read_dword_wrapper;
    //system->rsp.write_dword = &write_dword_wrapper;

    system->rsp.read_word = &read_rsp_word_wrapper;
    system->rsp.write_word = &write_rsp_word_wrapper;

    system->rsp.read_half = &read_rsp_half_wrapper;
    system->rsp.write_half = &write_rsp_half_wrapper;

    system->rsp.read_byte = &read_rsp_byte_wrapper;
    system->rsp.write_byte = &write_rsp_byte_wrapper;

    system->rsp.read_physical_byte = &read_physical_byte_wrapper;
    system->rsp.write_physical_byte = &write_physical_byte_wrapper;

    system->rsp.read_physical_word = &read_physical_word_wrapper;
    system->rsp.write_physical_word = &write_physical_word_wrapper;

    for (int i = 0; i < SP_IMEM_SIZE / 4; i++) {
        system->rsp.icache[i].instruction.raw = 0;
        system->rsp.icache[i].type = MIPS_UNKNOWN;
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

    global_system = system;
    if (enable_frontend) {
        render_init(system);
    }
    system->debugger_state.enabled = enable_debug;
    if (enable_debug) {
        debugger_init(system);
    }
    return system;
}

INLINE void _n64_system_step(n64_system_t* system) {
    if (system->debugger_state.enabled && check_breakpoint(&system->debugger_state, system->cpu.pc)) {
        debugger_breakpoint_hit(system);
    }
    while (system->debugger_state.broken) {
        usleep(1000);
        debugger_tick(system);
    }
    r4300i_step(&system->cpu);

    if (!system->rsp.status.halt) {
        rsp_step(system);
    }
}

void n64_system_step(n64_system_t* system) {
    _n64_system_step(system);
}

#define CYCLES_PER_INSTR 1

INLINE void check_vsync(n64_system_t* system) {
    if (system->vi.v_current == system->vi.vsync >> 1) {
        rdp_update_screen();
    }
}

void n64_system_loop(n64_system_t* system) {
    int cycles = 0;
    while (!should_quit) {
        for (system->vi.v_current = 0; system->vi.v_current < NUM_SHORTLINES; system->vi.v_current++) {
            check_vi_interrupt(system);
            check_vsync(system);
            while (cycles <= SHORTLINE_CYCLES) {
                _n64_system_step(system);
                cycles += CYCLES_PER_INSTR;
                system->debugger_state.steps = 0;
            }
            cycles -= SHORTLINE_CYCLES;
            ai_step(system, SHORTLINE_CYCLES);
        }
        for (; system->vi.v_current < NUM_SHORTLINES + NUM_LONGLINES; system->vi.v_current++) {
            check_vi_interrupt(system);
            check_vsync(system);
            while (cycles <= LONGLINE_CYCLES) {
                _n64_system_step(system);
                cycles += CYCLES_PER_INSTR;
                system->debugger_state.steps = 0;
            }
            cycles -= LONGLINE_CYCLES;
            ai_step(system, LONGLINE_CYCLES);
        }
        check_vi_interrupt(system);
        check_vsync(system);
        if (system->debugger_state.enabled) {
            debugger_tick(system);
        }
    }
}

void n64_system_cleanup(n64_system_t* system) {
    rdp_cleanup();
    debugger_cleanup(system);
    free(system);
}

void n64_request_quit() {
    should_quit = true;
}

void on_interrupt_change(n64_system_t* system) {
    bool interrupt = system->mi.intr.raw & system->mi.intr_mask.raw;
    logwarn("ip2 is now: %d", interrupt);
    system->cpu.cp0.cause.ip2 = interrupt;
}

void interrupt_raise(n64_system_t* system, n64_interrupt_t interrupt) {
    switch (interrupt) {
        case INTERRUPT_VI:
            logwarn("Raising VI interrupt");
            system->mi.intr.vi = true;
            break;
        case INTERRUPT_SI:
            logwarn("Raising SI interrupt");
            system->mi.intr.si = true;
            break;
        case INTERRUPT_PI:
            logwarn("Raising PI interrupt");
            system->mi.intr.pi = true;
            break;
        case INTERRUPT_AI:
            logwarn("Raising AI interrupt");
            system->mi.intr.ai = true;
            break;
        case INTERRUPT_DP:
            logwarn("Raising DP interrupt");
            system->mi.intr.dp = true;
            break;
        case INTERRUPT_SP:
            logwarn("Raising SP interrupt");
            system->mi.intr.sp = true;
            break;
        default:
            logfatal("Raising unimplemented interrupt: %d", interrupt);
    }

    on_interrupt_change(system);
}

void interrupt_lower(n64_system_t* system, n64_interrupt_t interrupt) {
    switch (interrupt) {
        case INTERRUPT_VI:
            system->mi.intr.vi = false;
            logwarn("Lowering VI interrupt");
            break;
        case INTERRUPT_SI:
            system->mi.intr.si = false;
            logwarn("Lowering SI interrupt");
            break;
        case INTERRUPT_PI:
            system->mi.intr.pi = false;
            logwarn("Lowering PI interrupt");
            break;
        case INTERRUPT_DP:
            system->mi.intr.dp = false;
            logwarn("Lowering DP interrupt");
            break;
        case INTERRUPT_AI:
            system->mi.intr.ai = false;
            logwarn("Lowering DP interrupt");
            break;
        case INTERRUPT_SP:
            system->mi.intr.sp = false;
            logwarn("Lowering SP interrupt");
            break;
        default:
            logfatal("Lowering unimplemented interrupt: %d", interrupt);
    }

    on_interrupt_change(system);
}
