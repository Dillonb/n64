#include <stdio.h>
#ifndef N64_WIN
#include <unistd.h>
#endif
#include <cflags.h>
#include <log.h>
#include <system/n64system.h>
#include <mem/pif.h>
#include <rdp/rdp.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <frontend/tas_movie.h>
#include <signal.h>
#include <imgui/imgui_ui.h>
#include "frontend.h"

void usage(cflags_t* flags) {
    cflags_print_usage(flags,
                       "[OPTION]... [FILE]",
                       "n64, a dgb n64 emulator",
                       "https://github.com/Dillonb/n64");
}

#ifndef N64_WIN
void sig_handler(int signum) {
    if (signum == SIGUSR1) {
        delayed_log_set_verbosity(LOG_VERBOSITY_DEBUG);
    } else if (signum == SIGUSR2) {
        delayed_log_set_verbosity(LOG_VERBOSITY_WARN);
    }
}
#endif

#define PIF_ROM_PATH (is_rom_pal(&n64sys.mem.rom) ? "pif.pal.rom" : "pif.rom")

INLINE bool file_exists(const char* path) {
#ifndef N64_WIN
    return access(PIF_ROM_PATH, F_OK) == 0;
#else
    FILE* f = fopen(path, "r");
    bool exists = false;
    if (f) {
        exists = true;
        fclose(f);
    }
    return exists;
#endif
}

int main(int argc, char** argv) {

#ifndef N64_WIN
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
#endif

    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");

    bool help = false;
    cflags_add_bool(flags, 'h', "help", &help, "Display this help message");

    bool interpreter = false;
    cflags_add_bool(flags, 'i', "interpreter", &interpreter, "Force the use of the interpreter");

    bool software_mode = false;
    cflags_add_bool(flags, 's', "software-mode", &software_mode, "Use software mode RDP (UNFINISHED!)");

    bool debug = false;
#ifdef N64_DEBUG_MODE
#ifndef N64_WIN
    char description[110];
    snprintf(description, sizeof(description), "Enable debug mode. Starts halted and listens on port %d for gdb. NOTE: implies -i!", GDB_CPU_PORT);
    cflags_add_bool(flags, 'd', "debug", &debug, description);
#endif
#endif

    const char* rdp_plugin_path = NULL;
    cflags_add_string(flags, 'r', "rdp", &rdp_plugin_path, "Load RDP plugin (Mupen64Plus compatible) "
                                                           "- note: disables UI and requires ROM to be passed on the command line!");
    const char* tas_movie_path = NULL;
    cflags_add_string(flags, 'm', "movie", &tas_movie_path, "Load movie (Mupen64Plus .m64 format)");

    const char* pif_rom_path = NULL;
    cflags_add_string(flags, 'p', "pif", &pif_rom_path, "Load PIF ROM");

    cflags_parse(flags, argc, argv);

    if (help) {
        usage(flags);
        return 0;
    }

    log_set_verbosity(verbose->count);
#ifdef N64_DEBUG_MODE
    // In debug builds, always log at least warnings.
    if (log_get_verbosity() < LOG_VERBOSITY_WARN) {
        log_set_verbosity(LOG_VERBOSITY_WARN);
    }
    // if debug mode is enabled, force the interpreter
    if (debug && !interpreter) {
        logwarn("Debug mode enabled, forcing the use of the interpreter!");
        interpreter = true;
    }
#endif
    if (rdp_plugin_path != NULL) {
        if (flags->argc != 1) {
            usage(flags);
            return 1;
        }
        init_n64system(flags->argv[0], true, debug, OPENGL_VIDEO_TYPE, interpreter);
        load_rdp_plugin(rdp_plugin_path);
    } else if (software_mode) {
        const char* rom_path = NULL;
        if (flags->argc >= 1) {
            rom_path = flags->argv[0];
        }
        init_n64system(rom_path, true, debug, SOFTWARE_VIDEO_TYPE, interpreter);
        init_softrdp(&n64sys.softrdp_state, (byte*)&n64sys.mem.rdram);
    } else {
        const char* rom_path = NULL;
        if (flags->argc >= 1) {
            rom_path = flags->argv[0];
        }
        init_n64system(rom_path, true, debug, VULKAN_VIDEO_TYPE, interpreter);
        load_parallel_rdp();
        load_imgui_ui();
        register_imgui_event_handler(imgui_handle_event);
    }
    if (tas_movie_path != NULL) {
        load_tas_movie(tas_movie_path);
    }
    if (pif_rom_path) {
        load_pif_rom(pif_rom_path);
    } else if (file_exists(PIF_ROM_PATH)) {
        logalways("Found PIF ROM at %s, loading", PIF_ROM_PATH);
        load_pif_rom(PIF_ROM_PATH);
    }
    if (n64sys.mem.rom.rom != NULL) {
        pif_rom_execute();
    }
#ifdef N64_DEBUG_MODE
#ifndef N64_WIN
    if (debug) {
        printf("Listening on 0.0.0.0:%d - Waiting for GDB to connect...\n", GDB_CPU_PORT);
        n64sys.debugger_state.broken = true;
    }
#endif
#endif
    cflags_free(flags);
    while (n64sys.mem.rom.rom == NULL && !n64_should_quit()) {
        update_screen_parallel_rdp_no_game();
    }
    n64_system_loop();
    n64_system_cleanup();
}