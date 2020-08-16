#include <stdio.h>
#include <cflags.h>
#include <log.h>
#include <system/n64system.h>
#include <mem/pif.h>
#include <rdp/rdp.h>

void usage(cflags_t* flags) {
    cflags_print_usage(flags,
                       "[OPTION]... FILE",
                       "n64, a dgb n64 emulator",
                       "https://github.com/Dillonb/n64");
}

int main(int argc, char** argv) {
    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");
    bool debug = false;
    char description[100];
    snprintf(description, sizeof(description), "Enable debug mode. Starts halted and listens on port %d for gdb.", GDB_CPU_PORT);
    cflags_add_bool(flags, 'd', "debug", &debug, description);
    const char* rdp_plugin_path = NULL;

    cflags_add_string(flags, 'r', "rdp", &rdp_plugin_path, "Load RDP plugin (Mupen64Plus compatible)");
    cflags_parse(flags, argc, argv);
    if (flags->argc != 1) {
        usage(flags);
        return 1;
    }
    log_set_verbosity(verbose->count);
    n64_system_t* system = init_n64system(flags->argv[0], true, debug);
    if (rdp_plugin_path != NULL) {
        load_rdp_plugin(system, rdp_plugin_path);
    } else {
        usage(flags);
        logdie("Running without loading an RDP plugin is not currently supported.")
    }
    pif_rom_execute(system);
    if (debug) {
        printf("Listening on 0.0.0.0:%d - Waiting for GDB to connect...\n", GDB_CPU_PORT);
        system->debugger_state.broken = true;
    }
    n64_system_loop(system);
    n64_system_cleanup(system);
}