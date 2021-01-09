#include <stdio.h>
#include <stdlib.h>
#define LOG_ENABLED
#include <log.h>
#include <system/n64system.h>
#include <cpu/mips_instructions.h>
#include <cpu/r4300i_register_access.h>
#include <mem/n64bus.h>

#define MAX_STEPS 10000000
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

    n64_system_t* system = init_n64system(argv[1], false, false, UNKNOWN_VIDEO_TYPE, false);
    // Normally handled by the bootcode, we gotta do it ourselves.
    for (int i = 0; i < 1048576; i++) {
        byte b = n64_read_byte(system, 0x10001000 + i);
        n64_write_byte(system, 0x00001000 + i, b);
    }

    set_pc_word_r4300i(&system->cpu, system->mem.rom.header.program_counter);

    loginfo("Initial PC: 0x%016lX\n", system->cpu.pc);

    int steps = 0;
    for (; steps < MAX_STEPS && !test_complete(system); steps++) {
        n64_system_step(system, false);
    }

    if (!test_complete(system)) {
        logfatal("Test timed out after %d steps\n", MAX_STEPS);
    }

    printf("SUCCESS: all tests passed! Took %d steps.\n", steps);
}