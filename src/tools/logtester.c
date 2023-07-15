#include <imgui/imgui_ui.h>
#include <stdio.h>
#include <cflags.h>
#include <rdp/rdp.h>
#include <cpu/rsp.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <interface/ai.h>
#include <interface/vi.h>
#include <util.h>
#include "log.h"
#include "system/n64system.h"
#include "mem/pif.h"

void usage(cflags_t* flags) {
    cflags_print_usage(flags,
                       "[OPTION]... FILE",
                       "n64, a dgb n64 emulator",
                       "https://github.com/Dillonb/n64");
}

void check_cpu_log(FILE* fp) {
    u64 expected_pc;
    u64 expected_gpr[32];

    int line = 0;
    while (true) {
        line++;
        fread(&expected_pc, sizeof(u64), 1, fp);
        fread(expected_gpr, sizeof(u64), 32, fp);
        bool bad = false;
        if (expected_pc != N64CPU.pc) {
          logalways("line %d: PC is wrong: expected %016lX but was %016lX", line, expected_pc, N64CPU.pc);
          bad = true;
        }
        for (int i = 0; i < 32; i++) {
            if (expected_gpr[i] != N64CPU.gpr[i]) {
              logalways("line %d: r%d is wrong: expected %016lX but was %016lX", line, i, expected_gpr[i], N64CPU.gpr[i]);
              bad = true;
            }
        }
        if (bad) {
            logfatal("Found a difference!");
        }
        n64_system_step(false, 1);
    }
}

int main(int argc, char** argv) {
    const char* log_file = NULL;

    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");
    cflags_add_string(flags, 'f', "log-file", &log_file, "log file to check run against");

    const char* pif_rom_path = NULL;
    cflags_add_string(flags, 'p', "pif", &pif_rom_path, "Load PIF ROM");

    cflags_parse(flags, argc, argv);

    if (flags->argc != 1) {
        usage(flags);
        return 1;
    }

    if (!log_file) {
        logfatal("Must pass a log file with -f!");
    }

    FILE* fp = fopen(log_file, "r");

    const char* rom = flags->argv[0];

    log_set_verbosity(verbose->count);

    init_n64system(rom, true, false, VULKAN_VIDEO_TYPE, false);
    prdp_init_internal_swapchain();
    load_imgui_ui();

    if (pif_rom_path) {
        load_pif_rom(pif_rom_path);
    }
    pif_rom_execute();

    check_cpu_log(fp);

    n64_system_cleanup();
}