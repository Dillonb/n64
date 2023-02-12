#include <stdio.h>
#include <log.h>
#include <system/n64system.h>
#include <mem/pif.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <imgui/imgui_ui.h>
#include <frontend/frontend.h>
#include <system/scheduler.h>
#include <disassemble.h>
#include <mem/n64bus.h>

n64_system_t n64sys_interpreter;
r4300i_t n64cpu_interpreter;
scheduler_t n64scheduler_interpreter;

n64_system_t n64sys_dynarec;
r4300i_t n64cpu_dynarec;
scheduler_t n64scheduler_dynarec;

bool compare() {
    bool good = true;
    good &= n64cpu_interpreter.pc == n64cpu_dynarec.pc;
    for (int i = 0; i < 32; i++) {
        good &= n64cpu_interpreter.gpr[i] == n64cpu_dynarec.gpr[i];
    }
    good &= memcmp(n64sys_interpreter.mem.rdram, n64sys_dynarec.mem.rdram, N64_RDRAM_SIZE) == 0;
    return good;
}

void print_state() {
    printf("PC: %016lX %016lX\n", n64cpu_interpreter.pc, n64cpu_dynarec.pc);
    for (int i = 0; i < 32; i++) {
        bool good = n64cpu_interpreter.gpr[i] == n64cpu_dynarec.gpr[i];
        printf("%s: %016lX %016lX\n", register_names[i], n64cpu_interpreter.gpr[i], n64cpu_dynarec.gpr[i]);
        if (!good) {
            printf("BAD!\n");
        }
    }

    for (int i = 0; i < N64_RDRAM_SIZE; i++) {
        if (n64sys_interpreter.mem.rdram[i] != n64sys_dynarec.mem.rdram[i]) {
            printf("%08X: %02X %02X\n", n64sys_interpreter.mem.rdram[i], n64sys_dynarec.mem.rdram[i]);
        }
    }
}

void copy_to(n64_system_t* sys, r4300i_t* cpu, scheduler_t* scheduler) {
    memcpy(sys, &n64sys, sizeof(n64_system_t));
    memcpy(cpu, &N64CPU, sizeof(r4300i_t));
    memcpy(scheduler, &n64scheduler, sizeof(scheduler_t));
}

void restore_from(n64_system_t* sys, r4300i_t* cpu, scheduler_t* scheduler) {
    memcpy(&n64sys, sys, sizeof(n64_system_t));
    memcpy(&N64CPU, cpu, sizeof(r4300i_t));
    memcpy(&n64scheduler, scheduler, sizeof(scheduler_t));
}

int main(int argc, char** argv) {
#ifndef INSTANT_PI_DMA
    logfatal("The emulator must be built with INSTANT_PI_DMA for this tool to be effective! (TODO: and probably other DMAs, too)");
#endif
    if (argc != 2) {
        logfatal("Usage: %s <rom>", argv[0]);
    }
    const char* rom_path = argv[1];

    // TODO: enable the UI
    init_n64system(rom_path, false, false, UNKNOWN_VIDEO_TYPE, false);
    /*
    prdp_init_internal_swapchain();
    load_imgui_ui();
    register_imgui_event_handler(imgui_handle_event);
    */
    n64_load_rom(rom_path);
    pif_rom_execute();

    copy_to(&n64sys_dynarec, &n64cpu_dynarec, &n64scheduler_dynarec);
    copy_to(&n64sys_interpreter, &n64cpu_interpreter, &n64scheduler_interpreter);

    u64 start_pc = 0;
    int steps = 0;
    do {
        restore_from(&n64sys_dynarec, &n64cpu_dynarec, &n64scheduler_dynarec);
        start_pc = n64cpu.pc;
        // Step
        steps = n64_system_step(true);
        copy_to(&n64sys_dynarec, &n64cpu_dynarec, &n64scheduler_dynarec);

        restore_from(&n64sys_interpreter, &n64cpu_interpreter, &n64scheduler_interpreter);
        // Step
        for (int i = 0; i < steps; i++) {
            n64_system_step(false);
        }
        copy_to(&n64sys_interpreter, &n64cpu_interpreter, &n64scheduler_interpreter);

    } while (compare());
    printf("Found a difference at pc: %016lX, ran for %d steps\n", start_pc, steps);
    printf("MIPS code:\n");
    u32 physical = resolve_virtual_address_or_die(start_pc, BUS_LOAD);
    if (physical >= N64_RDRAM_SIZE) {
        printf("outside of RDAM, can't disassemble (TODO)");
    }
    print_multi_guest(physical, &n64sys.mem.rdram[physical], steps * 4);
    print_state();
}