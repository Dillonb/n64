#include "n64system.h"
#include "common/log.h"
#include "n64mem.h"
#include "n64rom.h"

n64_system_t* init_n64system(const char* rom_path, bool enable_frontend) {
    n64_system_t* system = malloc(sizeof(n64_system_t));
    unimplemented(!enable_frontend, "Disabling the frontend is not yet supported")
    init_mem(&system->mem);
    load_n64rom(&system->mem.rom, rom_path);
}

void n64_system_loop(n64_system_t* system) {

}

void n64_system_cleanup(n64_system_t* system) {
    logfatal("No cleanup yet")
}
