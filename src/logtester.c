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
    int log_lines = -1;

    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");
    cflags_add_string(flags, 'f', "log-file", &log_file, "log file to check run against");
    cflags_add_int(flags, 'l', "num-log-lines", &log_lines, "number of lines in the file to check");

    cflags_parse(flags, argc, argv);

    if (flags->argc != 1) {
        usage(flags);
        return 1;
    }

    if (!log_file) {
        logfatal("Must pass a log file with -f!")
    }

    if (log_lines == -1) {
        logfatal("Must pass number of log lines with -l!")
    }

    FILE* fp = fopen(log_file, "r");

    const char* rom = flags->argv[0];

    log_set_verbosity(verbose->count);
    n64_system_t* system = init_n64system(rom, true);
    pif_rom_execute(system);
    char lastinstr[100];
    for (int line = 0; line < log_lines; line++) {
        char* regline = NULL;
        char* instrline = NULL;
        size_t len = 0;

        if (getline(&regline, &len, fp) == -1) {
            break;
        }

        loginfo_nonewline("Checking log line %d | %s", line + 1, regline)
        char* tok = strtok(regline, " ");
        for (int r = 0; r < 32; r++) {
            dword expected = strtol(tok, NULL, 16);
            tok = strtok(NULL, " ");
            dword actual = system->cpu.gpr[r];
            if (expected != actual) {
                logwarn("Failed running line: %s", lastinstr)
                logwarn("Line %d: $%s (r%d) expected: 0x%08lX actual: 0x%08lX", line + 1, register_names[r], r, expected, actual)
                logfatal("Line %d: $%s (r%d) expected: 0x%08lX actual: 0x%08lX", line + 1, register_names[r], r, expected, actual)
            }
        }

        if (getline(&instrline, &len, fp) == -1) {
            break;
        }

        loginfo_nonewline("Checking log line %d | %s", line + 1, instrline)

        strcpy(lastinstr, instrline);

        tok = strtok(instrline, " ");
        dword pc = strtol(tok, NULL, 16);
        if (pc != system->cpu.pc) {
            logfatal("Line %d: PC expected: 0x%08lX actual: 0x%08lX", line + 1, pc, system->cpu.pc)
        }
        n64_system_step(system);
    }
    n64_system_cleanup(system);
}