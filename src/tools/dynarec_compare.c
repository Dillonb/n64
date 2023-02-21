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
#include <cpu/dynarec/dynarec.h>
#include <rsp.h>

n64_system_t n64sys_interpreter;
r4300i_t n64cpu_interpreter;
rsp_t n64rsp_interpreter;
scheduler_t n64scheduler_interpreter;

n64_system_t n64sys_dynarec;
r4300i_t n64cpu_dynarec;
rsp_t n64rsp_dynarec;
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

void print_colorcoded_u64(const char* name, u64 expected, u64 actual) {
    printf("%4s 0x%016lX 0x", name, expected);
    for (int offset = 56; offset >= 0; offset -= 8) {
        u64 good_byte = (expected >> offset) & 0xFF;
        u64 bad_byte = (actual >> offset) & 0xFF;
        printf("%s%02X%s", good_byte == bad_byte ? "" : COLOR_RED, (u8)bad_byte, good_byte == bad_byte ? "" : COLOR_END);
    }
    printf("%s\n", expected == actual ? "" : " BAD!");
}

void print_state() {
    printf("expected (interpreter)  actual (dynarec)\n");
    print_colorcoded_u64("PC", n64cpu_interpreter.pc, n64cpu_dynarec.pc);
    for (int i = 0; i < 32; i++) {
        print_colorcoded_u64(register_names[i], n64cpu_interpreter.gpr[i], n64cpu_dynarec.gpr[i]);
    }

    for (int i = 0; i < N64_RDRAM_SIZE; i++) {
        if (n64sys_interpreter.mem.rdram[i] != n64sys_dynarec.mem.rdram[i]) {
            printf("%08X: %02X %02X\n", i, n64sys_interpreter.mem.rdram[i], n64sys_dynarec.mem.rdram[i]);
        }
    }
}

void copy_to(n64_system_t* sys, r4300i_t* cpu, rsp_t* rsp, scheduler_t* scheduler) {
    memcpy(sys, &n64sys, sizeof(n64_system_t));
    memcpy(cpu, &N64CPU, sizeof(r4300i_t));
    memcpy(rsp, &N64RSP, sizeof(rsp_t));
    memcpy(scheduler, &n64scheduler, sizeof(scheduler_t));
}

void restore_from(n64_system_t* sys, r4300i_t* cpu, rsp_t* rsp, scheduler_t* scheduler) {
    memcpy(&n64sys, sys, sizeof(n64_system_t));
    memcpy(&N64CPU, cpu, sizeof(r4300i_t));
    memcpy(&N64RSP, rsp, sizeof(rsp_t));
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
    init_n64system(rom_path, true, false, SOFTWARE_VIDEO_TYPE, false);
    softrdp_init(&n64sys.softrdp_state, (u8 *) &n64sys.mem.rdram);
    /*
    prdp_init_internal_swapchain();
    load_imgui_ui();
    register_imgui_event_handler(imgui_handle_event);
    */
    n64_load_rom(rom_path);
    pif_rom_execute();


    u64 start_comparing_at = (s32)n64sys.mem.rom.header.program_counter;

    while (N64CPU.pc != start_comparing_at) {
        n64_system_step(false, 1);
    }

    logalways("ROM booted to %016lX, beginning comparison", start_comparing_at);

    copy_to(&n64sys_dynarec, &n64cpu_dynarec, &n64rsp_dynarec, &n64scheduler_dynarec);
    copy_to(&n64sys_interpreter, &n64cpu_interpreter, &n64rsp_interpreter, &n64scheduler_interpreter);

    u64 start_pc = 0;
    int steps = 0;
    do {
        restore_from(&n64sys_dynarec, &n64cpu_dynarec, &n64rsp_dynarec, &n64scheduler_dynarec);
        if (n64cpu.pc != start_pc) {
            printf("Running compare at 0x%08X\n", (u32)n64cpu.pc);
        }
        start_pc = n64cpu.pc;
        // Step jit
        steps = n64_system_step(true, -1);
        copy_to(&n64sys_dynarec, &n64cpu_dynarec, &n64rsp_dynarec, &n64scheduler_dynarec);

        restore_from(&n64sys_interpreter, &n64cpu_interpreter, &n64rsp_interpreter, &n64scheduler_interpreter);
        // Step interpreter
        n64_system_step(false, steps);
        copy_to(&n64sys_interpreter, &n64cpu_interpreter, &n64rsp_interpreter, &n64scheduler_interpreter);

    } while (compare());
    printf("Found a difference at pc: %016lX, ran for %d steps\n", start_pc, steps);
    printf("MIPS code:\n");
    u32 physical = resolve_virtual_address_or_die(start_pc, BUS_LOAD);
    n64_dynarec_block_t* block = &n64sys.dynarec->blockcache[BLOCKCACHE_OUTER_INDEX(physical)][BLOCKCACHE_INNER_INDEX(physical)];
    if (physical >= N64_RDRAM_SIZE) {
        printf("outside of RDAM, can't disassemble (TODO)\n");
    } else {
        print_multi_guest(physical, &n64sys.mem.rdram[physical], block->guest_size);
    }
    printf("IR\n");
    print_ir_block();
    printf("Host code:\n");
    print_multi_host((uintptr_t)block->run, (u8*)block->run, block->host_size);
    print_state();
}