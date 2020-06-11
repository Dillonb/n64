#include "n64system.h"
#include "common/log.h"
#include "n64mem.h"
#include "n64rom.h"
#include "n64bus.h"

n64_system_t* global_system;

word read_word_wrapper(word address) {
    return n64_read_word(global_system, address);
}

void write_word_wrapper(word address, word value) {
    n64_write_word(global_system, address, value);
}

n64_system_t* init_n64system(const char* rom_path, bool enable_frontend) {
    n64_system_t* system = malloc(sizeof(n64_system_t));
    unimplemented(!enable_frontend, "Disabling the frontend is not yet supported")
    init_mem(&system->mem);
    load_n64rom(&system->mem.rom, rom_path);
    system->cpu.read_word = &read_word_wrapper;
    system->cpu.write_word = &write_word_wrapper;
    global_system = system;
    return system;
}

INLINE void n64_system_step(n64_system_t* system) {
    r4300i_step(&system->cpu);
}

_Noreturn void n64_system_loop(n64_system_t* system) {
    while (true) {
        n64_system_step(system);
    }
}

void n64_system_cleanup(n64_system_t* system) {
    logfatal("No cleanup yet")
}
