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
    const char* log_file = NULL;

    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");
    cflags_add_string(flags, 'f', "log-file", &log_file, "log file to check run against");

    cflags_parse(flags, argc, argv);

    if (flags->argc != 1) {
        usage(flags);
        return 1;
    }

    if (!log_file) {
        logfatal("Must pass a log file with -f!")
    }

    FILE* fp = fopen(log_file, "r");

    const char* rom = flags->argv[0];

    log_set_verbosity(verbose->count);
    n64_system_t* system = init_n64system(rom, true);
    pif_rom_execute(system);

    system->cpu.gpr[0] = 0x00000000;
    system->cpu.gpr[1] = 0x00000001;
    system->cpu.gpr[2] = 0x0ebda536;
    system->cpu.gpr[3] = 0x0ebda536;
    system->cpu.gpr[4] = 0x0000a536;
    system->cpu.gpr[5] = 0xc0f1d859;
    system->cpu.gpr[6] = 0xa4001f0c;
    system->cpu.gpr[7] = 0xa4001f08;
    system->cpu.gpr[8] = 0x000000f0;
    system->cpu.gpr[9] = 0x00000000;
    system->cpu.gpr[10] = 0x00000040;
    system->cpu.gpr[11] = 0xa4000040;
    system->cpu.gpr[12] = 0xed10d0b3;
    system->cpu.gpr[13] = 0x1402a4cc;
    system->cpu.gpr[14] = 0x2de108ea;
    system->cpu.gpr[15] = 0x3103e121;
    system->cpu.gpr[16] = 0x00000000;
    system->cpu.gpr[17] = 0x00000000;
    system->cpu.gpr[18] = 0x00000000;
    system->cpu.gpr[19] = 0x00000000;
    system->cpu.gpr[20] = 0x00000001;
    system->cpu.gpr[21] = 0x00000000;
    system->cpu.gpr[22] = 0x0000003f;
    system->cpu.gpr[23] = 0x00000000;
    system->cpu.gpr[24] = 0x00000000;
    system->cpu.gpr[25] = 0x9debb54f;
    system->cpu.gpr[26] = 0x00000000;
    system->cpu.gpr[27] = 0x00000000;
    system->cpu.gpr[28] = 0x00000000;
    system->cpu.gpr[29] = 0xa4001ff0;
    system->cpu.gpr[30] = 0x00000000;
    system->cpu.gpr[31] = 0xa4001550;


    char lastinstr[100];
    for (long line = 0; line < 0xFFFFFFFFFFFFFFFF; line++) {
        char* regline = NULL;
        char* instrline = NULL;
        size_t len = 0;

        if (getline(&regline, &len, fp) == -1) {
            break;
        }

        loginfo_nonewline("Checking log line %ld | %s", line + 1, regline)
        char* tok = strtok(regline, " ");
        for (int r = 0; r < 32; r++) {
            dword expected = strtol(tok, NULL, 16);
            tok = strtok(NULL, " ");
            dword actual = system->cpu.gpr[r] & 0xFFFFFFFF;
            if (expected != actual) {
                logwarn("Failed running line: %s", lastinstr)
                logwarn("Line %ld: $%s (r%d) expected: 0x%08lX actual: 0x%08lX", line + 1, register_names[r], r, expected, actual)
                logfatal("Line %ld: $%s (r%d) expected: 0x%08lX actual: 0x%08lX", line + 1, register_names[r], r, expected, actual)
            }
        }

        if (getline(&instrline, &len, fp) == -1) {
            break;
        }

        loginfo_nonewline("Checking log line %ld | %s", line + 1, instrline)

        strcpy(lastinstr, instrline);

        tok = strtok(instrline, " ");
        dword pc = strtol(tok, NULL, 16);
        if (pc != system->cpu.pc) {
            logfatal("Line %ld: PC expected: 0x%08lX actual: 0x%08X", line + 1, pc, system->cpu.pc)
        }
        n64_system_step(system);
    }
    n64_system_cleanup(system);
}