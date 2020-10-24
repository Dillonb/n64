#include <stdio.h>
#include <stdlib.h>
#define LOG_ENABLED
#include <log.h>
#include <system/n64system.h>
#include <mem/pif.h>
#include <cpu/mips_instructions.h>
#include <mem/dma.h>

#define MAX_STEPS 1000000
#define TEST_FAILED_REGISTER 30

bool test_complete(n64_system_t* system) {
    sdword test_failed = get_register(&system->cpu, TEST_FAILED_REGISTER);
    if (test_failed != 0) {
        if (test_failed != -1) {
            logfatal("Test #%ld failed.", test_failed);
        }

        return true;
    } else {
        return false;
    }
}

int main(int argc, char** argv) {
    if (argc == 0) {
        logfatal("Pass me a ROM file please");
    }

    log_set_verbosity(LOG_VERBOSITY_DEBUG);

    n64_system_t* system = init_n64system(argv[1], false, false);
    // Normally handled by the bootcode, we gotta do it ourselves.
    run_dma(system, 0x10001000, 0x00001000, 1048576, "CART to DRAM");

    set_pc_r4300i(&system->cpu, system->mem.rom.header.program_counter);

    loginfo("Initial PC: 0x%08X\n", system->cpu.pc);

    for (int steps = 0; steps < MAX_STEPS && !test_complete(system); steps++) {
        printf("pc: 0x%08X\n", system->cpu.pc);
        n64_system_step(system);
    }

    if (!test_complete(system)) {
        logfatal("Test timed out after %d steps\n", MAX_STEPS);
    }

    printf("SUCCESS: all tests passed!\n");
}