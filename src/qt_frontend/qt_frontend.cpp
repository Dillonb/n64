#include <QApplication>
#include <QCommandLineParser>
#include <settings.h>
#include <cflags.h>

#include "mainwindow.h"
extern "C" {
#include "perf_map_file.h"
#include "frontend/tas_movie.h"
}
#include "mem/pif.h"

void usage(cflags_t* flags) {
    cflags_print_usage(flags,
                       "[OPTION]... [FILE]",
                       "n64, a dgb n64 emulator",
                       "https://github.com/Dillonb/n64");
}

int main(int argc, char** argv) {
    n64_settings_init();
    QApplication app(argc, argv);

    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");

    bool help = false;
    cflags_add_bool(flags, 'h', "help", &help, "Display this help message");

#ifdef N64_DYNAREC_ENABLED
    bool interpreter = false;
    cflags_add_bool(flags, 'i', "interpreter", &interpreter, "Force the use of the interpreter");
#else
    bool interpreter = true;
#endif

    bool software_mode = false;
    cflags_add_bool(flags, 's', "software-mode", &software_mode, "Use software mode RDP (UNFINISHED!) (currently does nothing in the Qt frontend)");

    bool debug = false;
#ifdef N64_DEBUG_MODE
#ifndef N64_WIN
    char description[111];
    snprintf(description, sizeof(description), "Enable debug mode. Starts halted and listens on port defined in dgb-n64.ini for connections. NOTE: implies -i!");
    cflags_add_bool(flags, 'd', "debug", &debug, description);
#endif
#endif

    const char* tas_movie_path = NULL;
    cflags_add_string(flags, 'm', "movie", &tas_movie_path, "Load movie (Mupen64Plus .m64 format)");

    bool record_tas_movie = false;
    cflags_add_bool(flags, 'r', "record", &record_tas_movie, "Record movie instead of playing. -m must also be specified when this option is used!");

    const char* pif_rom_path = NULL;
    cflags_add_string(flags, 'p', "pif", &pif_rom_path, "Load PIF ROM");

    #ifdef __linux__
    bool perf_map = false;
    cflags_add_bool(flags, '\0', "perf-map", &perf_map, "Write a perf map file to /tmp for profiling JIT code");
    #endif

    cflags_parse(flags, argc, argv);

    #ifdef __linux__
    if (perf_map) {
        n64_perf_map_file_enable();
    }
    #endif

    if (record_tas_movie && tas_movie_path == NULL) {
        usage(flags);
        logdie("Must specify tas movie path (with -m) when recording a tas movie.");
    }
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

    const char* rom_path = NULL;
    if (flags->argc >= 1) {
        rom_path = flags->argv[0];
    }

    if (tas_movie_path != NULL) {
        if (record_tas_movie) {
            start_tas_recording(tas_movie_path);
        } else {
            load_tas_movie(tas_movie_path);
        }
    }
    if (pif_rom_path) {
        load_pif_rom(pif_rom_path);
    } else if (file_exists(PIF_ROM_PATH)) {
        logalways("Found PIF ROM at %s, loading", PIF_ROM_PATH);
        load_pif_rom(PIF_ROM_PATH);
    }

#ifdef N64_DEBUG_MODE
    if (debug) {
        if (n64_settings.http_api_port == 0) {
            logfatal("Debug mode enabled, but HTTP API not enabled! Please configure in dgb-n64.ini");
        }
        printf("Waiting for debugger to connect...\n");
        n64sys.debugger_state.broken = true;
    }
#endif

    cflags_free(flags);
    MainWindow mw(rom_path, debug, interpreter);
    mw.show();
    return app.exec();
}