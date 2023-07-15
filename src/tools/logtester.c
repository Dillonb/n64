#include <imgui/imgui_ui.h>
#include <r4300i.h>
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
    cp0_cause_t expected_cp0_cause;
    mi_intr_t expected_mi_intr;

    int line = 0;
    while (true) {
        line++;
        fread(&expected_pc, sizeof(u64), 1, fp);
        fread(expected_gpr, sizeof(u64), 32, fp);
        fread(&expected_cp0_cause.raw, sizeof(u32), 1, fp);
        fread(&expected_mi_intr.raw, sizeof(u32), 1, fp);
        bool bad = false;
        if (expected_pc != N64CPU.pc) {
          logalways("PC is wrong: expected %016lX but was %016lX", expected_pc, N64CPU.pc);
          bad = true;
        }
        for (int i = 0; i < 32; i++) {
            if (expected_gpr[i] != N64CPU.gpr[i]) {
              logalways("r%d is wrong: expected %016lX but was %016lX", i, expected_gpr[i], N64CPU.gpr[i]);
              bad = true;
            }
        }
        if (expected_cp0_cause.raw != N64CP0.cause.raw) {
          logalways("CP0 Cause is wrong: expected %08X but was %08X", expected_cp0_cause.raw, N64CP0.cause.raw);
          if (expected_cp0_cause.exception_code != N64CP0.cause.exception_code) logalways("expected exception_code: %d actual: %d", expected_cp0_cause.exception_code, N64CP0.cause.exception_code);
          if (expected_cp0_cause.ip0 != N64CP0.cause.ip0) logalways("expected ip0: %d actual: %d", expected_cp0_cause.ip0, N64CP0.cause.ip0);
          if (expected_cp0_cause.ip1 != N64CP0.cause.ip1) logalways("expected ip1: %d actual: %d", expected_cp0_cause.ip1, N64CP0.cause.ip1);
          if (expected_cp0_cause.ip2 != N64CP0.cause.ip2) logalways("expected ip2: %d actual: %d", expected_cp0_cause.ip2, N64CP0.cause.ip2);
          if (expected_cp0_cause.ip3 != N64CP0.cause.ip3) logalways("expected ip3: %d actual: %d", expected_cp0_cause.ip3, N64CP0.cause.ip3);
          if (expected_cp0_cause.ip4 != N64CP0.cause.ip4) logalways("expected ip4: %d actual: %d", expected_cp0_cause.ip4, N64CP0.cause.ip4);
          if (expected_cp0_cause.ip5 != N64CP0.cause.ip5) logalways("expected ip5: %d actual: %d", expected_cp0_cause.ip5, N64CP0.cause.ip5);
          if (expected_cp0_cause.ip6 != N64CP0.cause.ip6) logalways("expected ip6: %d actual: %d", expected_cp0_cause.ip6, N64CP0.cause.ip6);
          if (expected_cp0_cause.ip7 != N64CP0.cause.ip7) logalways("expected ip7: %d actual: %d", expected_cp0_cause.ip7, N64CP0.cause.ip7);
          if (expected_cp0_cause.coprocessor_error != N64CP0.cause.coprocessor_error) logalways("expected coprocessor_error: %d actual: %d", expected_cp0_cause.coprocessor_error, N64CP0.cause.coprocessor_error);
          if (expected_cp0_cause.branch_delay != N64CP0.cause.branch_delay) logalways("expected branch_delay: %d actual: %d", expected_cp0_cause.branch_delay, N64CP0.cause.branch_delay);

          bad = true;
        }
        if (expected_mi_intr.raw != n64sys.mi.intr.raw) {
          logalways("MI intr is wrong: expected %08X but was %08X", expected_mi_intr.raw, n64sys.mi.intr.raw);
          if (expected_mi_intr.sp != n64sys.mi.intr.sp) logalways("expected sp: %d actual: %d", expected_mi_intr.sp, n64sys.mi.intr.sp);
          if (expected_mi_intr.si != n64sys.mi.intr.si) logalways("expected si: %d actual: %d", expected_mi_intr.si, n64sys.mi.intr.si);
          if (expected_mi_intr.ai != n64sys.mi.intr.ai) logalways("expected ai: %d actual: %d", expected_mi_intr.ai, n64sys.mi.intr.ai);
          if (expected_mi_intr.vi != n64sys.mi.intr.vi) logalways("expected vi: %d actual: %d", expected_mi_intr.vi, n64sys.mi.intr.vi);
          if (expected_mi_intr.pi != n64sys.mi.intr.pi) logalways("expected pi: %d actual: %d", expected_mi_intr.pi, n64sys.mi.intr.pi);
          if (expected_mi_intr.dp != n64sys.mi.intr.dp) logalways("expected dp: %d actual: %d", expected_mi_intr.dp, n64sys.mi.intr.dp);
          //bad = true;
        }
        if (bad) {
            logfatal("Found a difference at line %d!", line);
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