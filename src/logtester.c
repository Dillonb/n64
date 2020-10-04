#include <stdio.h>
#include <cflags.h>
#include <rdp/rdp.h>
#include <cpu/rsp.h>
#include "log.h"
#include "system/n64system.h"
#include "mem/pif.h"

void usage(cflags_t* flags) {
    cflags_print_usage(flags,
                       "[OPTION]... FILE",
                       "n64, a dgb n64 emulator",
                       "https://github.com/Dillonb/n64");
}

void load_vecr(char *tok, vu_reg_t* v) {
    for (int e = 0; e < 8; e++) {
        half elem = strtol(tok + e * (5), NULL, 16);
        v->elements[7 - e] = elem;
    }
}

void fix_fake_vecr(vu_reg_t* v) {
    for (int e = 0; e < 8; e++) {
        switch (v->elements[e]) {
            case 0:
                v->elements[e] = 0;
                break;
            case 1:
            case 0xFFFF:
                v->elements[e] = 0xFFFF;
                break;
            default:
                logfatal("Invalid fake vector value: 0x%04X. WTF?", v->elements[e]);
        }
    }
}

bool compare_vecr(char *tok, char* name, vu_reg_t* reg) {
    for (int e = 0; e < 8; e++) {
        half expected = strtol(tok + e * (5), NULL, 16);
        half actual = reg->elements[7 - e];

        if (expected != actual) {
            printf("%s expected: %s\n", name, tok);
            printf("%s actual:   ", name);
            for (int ee = 0; ee < 8; ee++) {
                printf("%04x", reg->elements[7 - ee]);
                if (ee != 7) {
                    printf("|");
                }
            }
            printf("\n\n");
            return false;
        }
    }
    return true;
}

void check_rsp_log(n64_system_t* system, FILE* fp) {
    char pre_line_buf[3000];
    char post_line_buf[3000];
    char* line = NULL;
    size_t len = 0;

    int linenum = 0;

    while (true) {
        if (getline(&line, &len, fp) == -1) {
            printf("Reached the end of the file, line %d", linenum);
            exit(0);
        }

        linenum++;

        strcpy(pre_line_buf, line);
        char* pre_line = pre_line_buf;

        if (getline(&line, &len, fp) == -1) {
            printf("Reached the end of the file, line %d", linenum);
            exit(0);
        }

        linenum++;

        printf("Checking line %d\n", linenum);

        strcpy(post_line_buf, line);
        char* post_line = post_line_buf;

        system->rsp.pc = strtol(post_line, NULL, 16);
        word instr = strtol(post_line + 10, NULL, 16);
        system->rsp.write_physical_word((system->rsp.pc & 0xFFF) + SREGION_SP_IMEM, instr);

        pre_line += 59; // Skip all the other stuff and get right to regs
        post_line += 59; // Skip all the other stuff and get right to regs

        char* tok;
        strtok(pre_line, " ");

        for (int r = 0; r < 32; r++) {
            tok = strtok(NULL, " ");
            system->rsp.gpr[r] = strtol(tok, NULL, 16);
            strtok(NULL, " ");
        }

        for (int v = 0; v < 32; v++) {
            tok = strtok(NULL, " ");
            load_vecr(tok, &(system->rsp.vu_regs[v]));
            strtok(NULL, " ");
        }

        // ACC_H
        tok = strtok(NULL, " ");
        load_vecr(tok, &system->rsp.acc.h);
        strtok(NULL, " ");

        // ACC_M
        tok = strtok(NULL, " ");
        load_vecr(tok, &system->rsp.acc.m);
        strtok(NULL, " ");

        // ACC_L
        tok = strtok(NULL, " ");
        load_vecr(tok, &system->rsp.acc.l);
        strtok(NULL, " ");

        // VCC_H
        tok = strtok(NULL, " ");
        load_vecr(tok, &system->rsp.vcc.h);
        fix_fake_vecr(&system->rsp.vcc.h);
        strtok(NULL, " ");

        // VCC_L
        tok = strtok(NULL, " ");
        load_vecr(tok, &system->rsp.vcc.l);
        fix_fake_vecr(&system->rsp.vcc.l);
        strtok(NULL, " ");

        // VCO_H
        tok = strtok(NULL, " ");
        load_vecr(tok, &system->rsp.vco.h);
        fix_fake_vecr(&system->rsp.vco.h);
        strtok(NULL, " ");

        // VCO_L
        tok = strtok(NULL, " ");
        load_vecr(tok, &system->rsp.vco.l);
        fix_fake_vecr(&system->rsp.vco.l);
        strtok(NULL, " ");

        // VCE
        tok = strtok(NULL, " ");
        load_vecr(tok, &system->rsp.vce);
        fix_fake_vecr(&system->rsp.vce);
        strtok(NULL, " ");

        // Pre-setup is done, run the step
        rsp_step(system);

        bool all_correct = true;

        // Start reading post_line
        strtok(post_line, " ");

        for (int r = 0; r < 32; r++) {
            tok = strtok(NULL, " ");
            word actual = system->rsp.gpr[r];
            word expected = strtol(tok, NULL, 16);
            if (actual != expected) {
                logfatal("r%d expected: 0x%08X actual 0x%08X\n", r, expected, actual);
            }
            strtok(NULL, " ");
        }

        for (int v = 0; v < 32; v++) {
            tok = strtok(NULL, " ");
            char bufname[6];
            sprintf(bufname, "v%02d  ", v);
            all_correct &= compare_vecr(tok, bufname, &system->rsp.vu_regs[v]);
            strtok(NULL, " ");
        }

        // ACC_H
        tok = strtok(NULL, " ");
        all_correct &= compare_vecr(tok, "ACC_H", &system->rsp.acc.h);
        strtok(NULL, " ");

        // ACC_M
        tok = strtok(NULL, " ");
        all_correct &= compare_vecr(tok, "ACC_M", &system->rsp.acc.m);
        strtok(NULL, " ");

        // ACC_L
        tok = strtok(NULL, " ");
        all_correct &= compare_vecr(tok, "ACC_L", &system->rsp.acc.l);
        strtok(NULL, " ");

        // VCC_H
        tok = strtok(NULL, " ");
        fix_fake_vecr(&system->rsp.vcc.h);
        all_correct &= compare_vecr(tok, "VCC_H", &system->rsp.vcc.h);
        strtok(NULL, " ");

        // VCC_L
        tok = strtok(NULL, " ");
        fix_fake_vecr(&system->rsp.vcc.l);
        all_correct &= compare_vecr(tok, "VCC_L", &system->rsp.vcc.l);
        strtok(NULL, " ");

        // VCO_H
        tok = strtok(NULL, " ");
        fix_fake_vecr(&system->rsp.vco.h);
        all_correct &= compare_vecr(tok, "VCO_H", &system->rsp.vco.h);
        strtok(NULL, " ");

        // VCO_L
        tok = strtok(NULL, " ");
        fix_fake_vecr(&system->rsp.vco.l);
        all_correct &= compare_vecr(tok, "VCO_L", &system->rsp.vco.l);
        strtok(NULL, " ");

        // VCE
        tok = strtok(NULL, " ");
        fix_fake_vecr(&system->rsp.vce);
        all_correct &= compare_vecr(tok, "VCE", &system->rsp.vce);
        strtok(NULL, " ");

        if (!all_correct) {
            logfatal("Log mismatch!");
        }

    }
}

void check_cpu_log(n64_system_t* system, FILE* fp) {
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

        loginfo_nonewline("Checking log line %ld | %s", line + 1, regline);
        char* tok = strtok(regline, " ");
        for (int r = 0; r < 32; r++) {
            dword expected = strtol(tok, NULL, 16);
            tok = strtok(NULL, " ");
            dword actual = system->cpu.gpr[r] & 0xFFFFFFFF;
            if (expected != actual) {
                logwarn("Failed running line: %s", lastinstr);
                logwarn("Line %ld: $%s (r%d) expected: 0x%08lX actual: 0x%08lX", line + 1, register_names[r], r, expected, actual);
                logfatal("Line %ld: $%s (r%d) expected: 0x%08lX actual: 0x%08lX", line + 1, register_names[r], r, expected, actual);
            }
        }

        if (getline(&instrline, &len, fp) == -1) {
            break;
        }

        loginfo_nonewline("Checking log line %ld | %s", line + 1, instrline);

        strcpy(lastinstr, instrline);

        tok = strtok(instrline, " ");
        dword pc = strtol(tok, NULL, 16);
        if (pc != system->cpu.pc) {
            logfatal("Line %ld: PC expected: 0x%08lX actual: 0x%08X", line + 1, pc, system->cpu.pc);
        }
        n64_system_step(system);
    }
}

int main(int argc, char** argv) {
    const char* log_file = NULL;
    const char* rdp_plugin_path;
    bool test_rsp = false;

    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");
    cflags_add_string(flags, 'f', "log-file", &log_file, "log file to check run against");
    cflags_add_bool(flags, 's', "rsp", &test_rsp, "check RSP log file instead of CPU log file");
    cflags_add_string(flags, 'r', "rdp", &rdp_plugin_path, "Load RDP plugin (Mupen64Plus compatible)");

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
    n64_system_t* system = init_n64system(rom, true, false, UNKNOWN);

    if (rdp_plugin_path != NULL) {
        load_rdp_plugin(system, rdp_plugin_path);
    } else {
        usage(flags);
        logdie("Running without loading an RDP plugin is not currently supported.");
    }
    pif_rom_execute(system);

    if (test_rsp) {
        check_rsp_log(system, fp);
    } else {
        check_cpu_log(system, fp);
    }

    n64_system_cleanup(system);
}