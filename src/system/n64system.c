#include "n64system.h"
#include "../mem/n64bus.h"
#include "../render.h"

#define CPU_HERTZ 93750000
#define CPU_CYCLES_PER_FRAME (CPU_HERTZ / 60)

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

dword read_dword_wrapper(word address) {
    return n64_read_dword(global_system, address);
}

void write_dword_wrapper(word address, dword value) {
    n64_write_dword(global_system, address, value);
}

word read_word_wrapper(word address) {
    return n64_read_word(global_system, address);
}

void write_word_wrapper(word address, word value) {
    n64_write_word(global_system, address, value);
}

half read_half_wrapper(word address) {
    return n64_read_half(global_system, address);
}

void write_half_wrapper(word address, half value) {
    n64_write_half(global_system, address, value);
}

byte read_byte_wrapper(word address) {
    return n64_read_byte(global_system, address);
}

void write_byte_wrapper(word address, byte value) {
    n64_write_byte(global_system, address, value);
}

n64_system_t* init_n64system(const char* rom_path, bool enable_frontend) {
    n64_system_t* system = malloc(sizeof(n64_system_t));
    unimplemented(!enable_frontend, "Disabling the frontend is not yet supported")
    init_mem(&system->mem);
    load_n64rom(&system->mem.rom, rom_path);

    system->cpu.read_dword = &read_dword_wrapper;
    system->cpu.write_dword = &write_dword_wrapper;

    system->cpu.read_word = &read_word_wrapper;
    system->cpu.write_word = &write_word_wrapper;

    system->cpu.read_half = &read_half_wrapper;
    system->cpu.write_half = &write_half_wrapper;

    system->cpu.read_byte = &read_byte_wrapper;
    system->cpu.write_byte = &write_byte_wrapper;

    system->rsp_status.halt = true; // RSP starts halted

    global_system = system;
    render_init();
    return system;
}

INLINE void _n64_system_step(n64_system_t* system) {
    r4300i_step(&system->cpu);
    unimplemented(!system->rsp_status.halt, "RSP running!")
}

void n64_system_step(n64_system_t* system) {
    _n64_system_step(system);
}

void n64_system_loop(n64_system_t* system) {
    while (!should_quit) {
        system->vi.v_current = 0;
        for (; system->vi.v_current < NUM_SHORTLINES; system->vi.v_current++) {
            for (int i = 0; i < SHORTLINE_CYCLES; i++) {
                _n64_system_step(system);
            }
        }
        for (; system->vi.v_current < NUM_SHORTLINES + NUM_LONGLINES; system->vi.v_current++) {
            for (int i = 0; i < LONGLINE_CYCLES; i++) {
                _n64_system_step(system);
            }
        }
        render_screen(system);
    }
}

void n64_system_cleanup(n64_system_t* system) {
    logfatal("No cleanup yet")
}

void n64_request_quit() {
    should_quit = true;
}
