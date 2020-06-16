#include "n64system.h"
#include "../common/log.h"
#include "../mem/n64mem.h"
#include "../mem/n64rom.h"
#include "../mem/n64bus.h"

n64_system_t* global_system;

word read_word_wrapper(word address) {
    return n64_read_word(global_system, address);
}

void write_word_wrapper(word address, word value) {
    n64_write_word(global_system, address, value);
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
    system->cpu.read_word = &read_word_wrapper;
    system->cpu.write_word = &write_word_wrapper;

    system->cpu.read_byte = &read_byte_wrapper;
    system->cpu.write_byte = &write_byte_wrapper;

    system->rsp_status.halt = true; // RSP starts halted

    global_system = system;
    return system;
}

INLINE void _n64_system_step(n64_system_t* system) {
    r4300i_step(&system->cpu);
    unimplemented(!system->rsp_status.halt, "RSP running!")
}

void n64_system_step(n64_system_t* system) {
    _n64_system_step(system);
}

_Noreturn void n64_system_loop(n64_system_t* system) {
    while (true) {
        _n64_system_step(system);
    }
}

void n64_system_cleanup(n64_system_t* system) {
    logfatal("No cleanup yet")
}
