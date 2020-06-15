#include <stdio.h>
#include <cflags.h>
#include "common/log.h"
#include "system/n64system.h"
#include "mem/pif_rom.h"

void usage(cflags_t* flags) {
    cflags_print_usage(flags,
                       "[OPTION]... FILE",
                       "n64, a dgb n64 emulator",
                       "https://github.com/Dillonb/n64");
}

int main(int argc, char** argv) {
    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");
    cflags_parse(flags, argc, argv);
    if (flags->argc != 1) {
        usage(flags);
        return 1;
    }
    log_set_verbosity(verbose->count);
    n64_system_t* system = init_n64system(flags->argv[0], true);
    pif_rom_execute(system);
    n64_system_loop(system);
    n64_system_cleanup(system);
}