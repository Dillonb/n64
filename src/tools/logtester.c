#include <stdio.h>
#include <cflags.h>
#include <rdp/rdp.h>
#include <cpu/rsp.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <interface/ai.h>
#include <interface/vi.h>
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

void check_rsp_log(FILE* fp) {
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

        N64RSP.pc = strtol(post_line, NULL, 16);
        word instr = strtol(post_line + 10, NULL, 16);
        n64_write_physical_word((N64RSP.pc & 0xFFF) + SREGION_SP_IMEM, instr);

        pre_line += 59; // Skip all the other stuff and get right to regs
        post_line += 59; // Skip all the other stuff and get right to regs

        char* tok;
        strtok(pre_line, " ");

        for (int r = 0; r < 32; r++) {
            tok = strtok(NULL, " ");
            N64RSP.gpr[r] = strtol(tok, NULL, 16);
            strtok(NULL, " ");
        }

        for (int v = 0; v < 32; v++) {
            tok = strtok(NULL, " ");
            load_vecr(tok, &(N64RSP.vu_regs[v]));
            strtok(NULL, " ");
        }

        // ACC_H
        tok = strtok(NULL, " ");
        load_vecr(tok, &N64RSP.acc.h);
        strtok(NULL, " ");

        // ACC_M
        tok = strtok(NULL, " ");
        load_vecr(tok, &N64RSP.acc.m);
        strtok(NULL, " ");

        // ACC_L
        tok = strtok(NULL, " ");
        load_vecr(tok, &N64RSP.acc.l);
        strtok(NULL, " ");

        // VCC_H
        tok = strtok(NULL, " ");
        load_vecr(tok, &N64RSP.vcc.h);
        fix_fake_vecr(&N64RSP.vcc.h);
        strtok(NULL, " ");

        // VCC_L
        tok = strtok(NULL, " ");
        load_vecr(tok, &N64RSP.vcc.l);
        fix_fake_vecr(&N64RSP.vcc.l);
        strtok(NULL, " ");

        // VCO_H
        tok = strtok(NULL, " ");
        load_vecr(tok, &N64RSP.vco.h);
        fix_fake_vecr(&N64RSP.vco.h);
        strtok(NULL, " ");

        // VCO_L
        tok = strtok(NULL, " ");
        load_vecr(tok, &N64RSP.vco.l);
        fix_fake_vecr(&N64RSP.vco.l);
        strtok(NULL, " ");

        // VCE
        tok = strtok(NULL, " ");
        load_vecr(tok, &N64RSP.vce);
        fix_fake_vecr(&N64RSP.vce);
        strtok(NULL, " ");

        // Pre-setup is done, run the step
        rsp_step();

        bool all_correct = true;

        // Start reading post_line
        strtok(post_line, " ");

        for (int r = 0; r < 32; r++) {
            tok = strtok(NULL, " ");
            word actual = N64RSP.gpr[r];
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
            all_correct &= compare_vecr(tok, bufname, &N64RSP.vu_regs[v]);
            strtok(NULL, " ");
        }

        // ACC_H
        tok = strtok(NULL, " ");
        all_correct &= compare_vecr(tok, "ACC_H", &N64RSP.acc.h);
        strtok(NULL, " ");

        // ACC_M
        tok = strtok(NULL, " ");
        all_correct &= compare_vecr(tok, "ACC_M", &N64RSP.acc.m);
        strtok(NULL, " ");

        // ACC_L
        tok = strtok(NULL, " ");
        all_correct &= compare_vecr(tok, "ACC_L", &N64RSP.acc.l);
        strtok(NULL, " ");

        // VCC_H
        tok = strtok(NULL, " ");
        fix_fake_vecr(&N64RSP.vcc.h);
        all_correct &= compare_vecr(tok, "VCC_H", &N64RSP.vcc.h);
        strtok(NULL, " ");

        // VCC_L
        tok = strtok(NULL, " ");
        fix_fake_vecr(&N64RSP.vcc.l);
        all_correct &= compare_vecr(tok, "VCC_L", &N64RSP.vcc.l);
        strtok(NULL, " ");

        // VCO_H
        tok = strtok(NULL, " ");
        fix_fake_vecr(&N64RSP.vco.h);
        all_correct &= compare_vecr(tok, "VCO_H", &N64RSP.vco.h);
        strtok(NULL, " ");

        // VCO_L
        tok = strtok(NULL, " ");
        fix_fake_vecr(&N64RSP.vco.l);
        all_correct &= compare_vecr(tok, "VCO_L", &N64RSP.vco.l);
        strtok(NULL, " ");

        // VCE
        tok = strtok(NULL, " ");
        fix_fake_vecr(&N64RSP.vce);
        all_correct &= compare_vecr(tok, "VCE", &N64RSP.vce);
        strtok(NULL, " ");

        if (!all_correct) {
            logfatal("Log mismatch!");
        }

    }
}

void check_cpu_log(FILE* fp) {
    N64CPU.gpr[0] = 0x00000000;
    N64CPU.gpr[1] = 0x00000001;
    N64CPU.gpr[2] = 0x0ebda536;
    N64CPU.gpr[3] = 0x0ebda536;
    N64CPU.gpr[4] = 0x0000a536;
    N64CPU.gpr[5] = 0xc0f1d859;
    N64CPU.gpr[6] = 0xa4001f0c;
    N64CPU.gpr[7] = 0xa4001f08;
    N64CPU.gpr[8] = 0x000000f0;
    N64CPU.gpr[9] = 0x00000000;
    N64CPU.gpr[10] = 0x00000040;
    N64CPU.gpr[11] = 0xa4000040;
    N64CPU.gpr[12] = 0xed10d0b3;
    N64CPU.gpr[13] = 0x1402a4cc;
    N64CPU.gpr[14] = 0x2de108ea;
    N64CPU.gpr[15] = 0x3103e121;
    N64CPU.gpr[16] = 0x00000000;
    N64CPU.gpr[17] = 0x00000000;
    N64CPU.gpr[18] = 0x00000000;
    N64CPU.gpr[19] = 0x00000000;
    N64CPU.gpr[20] = 0x00000001;
    N64CPU.gpr[21] = 0x00000000;
    N64CPU.gpr[22] = 0x0000003f;
    N64CPU.gpr[23] = 0x00000000;
    N64CPU.gpr[24] = 0x00000000;
    N64CPU.gpr[25] = 0x9debb54f;
    N64CPU.gpr[26] = 0x00000000;
    N64CPU.gpr[27] = 0x00000000;
    N64CPU.gpr[28] = 0x00000000;
    N64CPU.gpr[29] = 0xa4001ff0;
    N64CPU.gpr[30] = 0x00000000;
    N64CPU.gpr[31] = 0xa4001550;


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
            dword actual = N64CPU.gpr[r] & 0xFFFFFFFF;
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
        if (pc != N64CPU.pc) {
            logfatal("Line %ld: PC expected: 0x%08lX actual: 0x%08lX", line + 1, pc, N64CPU.pc);
        }
        n64_system_step(false);
    }
}

void cpu_step(r4300i_t* cpu) {
    dword pc = cpu->pc;
    mips_instruction_t instruction;
    instruction.raw = n64_read_word(pc);

    cpu->prev_pc = cpu->pc;
    cpu->pc = cpu->next_pc;
    cpu->next_pc += 4;
    cpu->branch = false;

    r4300i_instruction_decode(pc, instruction)(instruction);
    cpu->exception = false; // only used in dynarec
}

void update_count(int taken) {
    r4300i_t* cpu = &N64CPU;

    uint64_t oldcount = cpu->cp0.count >> 1;
    uint64_t newcount = (cpu->cp0.count + (taken * CYCLES_PER_INSTR)) >> 1;
    if (unlikely(oldcount < cpu->cp0.compare && newcount >= cpu->cp0.compare)) {
        cpu->cp0.cause.ip7 = true;
        loginfo("Compare interrupt!");
        r4300i_interrupt_update(cpu);
    }
    cpu->cp0.count += taken;

}

int run_system_check_interrupt() {
    r4300i_t* cpu = &N64CPU;

    if (unlikely(cpu->interrupts > 0)) {
        if(cpu->cp0.status.ie && !cpu->cp0.status.exl && !cpu->cp0.status.erl) {
            r4300i_handle_exception(cpu->pc, EXCEPTION_INTERRUPT, 0);
            cpu->cp0.count += CYCLES_PER_INSTR;
            printf("Interrupt!\n");
            return CYCLES_PER_INSTR;
        }
    }
    return 0;
}

int run_system_and_check(long taken, char* line, long linenum) {
    r4300i_t* cpu = &N64CPU;
    printf("Running for %ld cycles on line %ld\n", taken, linenum);

    char* tok = strtok(NULL, " ");
    word expected_pc = strtol(tok, NULL, 16);

    static int cpu_steps = 0;
    for (int i = 0; i < taken /*&& cpu->pc != expected_pc*/; i++) {
        cpu_step(cpu);
    }
    cpu_steps += taken;

    if (expected_pc != cpu->pc) {
        logfatal("RIP! on line %ld, after a block of size %ld, PC expected 0x%08X actual 0x%08lX\n", linenum, taken, expected_pc, cpu->pc);
    }

    logalways("Synchronized at PC=0x%016lX, checking registers", cpu->pc);

    for (int r = 0; r < 32; r++) {
        tok = strtok(NULL, " ");
        dword expected = strtoul(tok, NULL, 16);
        dword actual = N64CPU.gpr[r];
        bool anybad = false;
        if (expected != actual) {
            logalways("RIP! on line %ld, after a block of size %ld, r%d (%s) expected 0x%016lX actual 0x%016lX\n", linenum, taken, r, register_names[r], expected, actual);
            anybad = true;
        }
        if (anybad) {
            logfatal("Encountered log differences.");
        }

    }


    if (!N64RSP.status.halt) {
        // 2 RSP steps per 3 CPU steps
        N64RSP.steps += (cpu_steps / 3) * 2;
        cpu_steps -= cpu_steps % 3;

        rsp_run();
    } else {
        cpu_steps = 0;
    }

    update_count(taken);
    return taken;
}

void check_jit_sync_log(FILE* fp) {
    int cycles = 0;
    long linenum = 0;
    for (int frame = 0; frame < 0xFFFFFFFF; frame++) {
        for (n64sys.vi.v_current = 0; n64sys.vi.v_current < NUM_SHORTLINES; n64sys.vi.v_current++) {
            check_vi_interrupt();
            check_vsync();
            while (cycles <= SHORTLINE_CYCLES) {
                cycles += run_system_check_interrupt();
                if (cycles > SHORTLINE_CYCLES) {
                    break;
                }
                char *line = NULL;
                size_t len = 0;

                if (getline(&line, &len, fp) == -1) {
                    logalways("End of file.");
                    exit(0);
                }

                char* tok = strtok(line, " ");
                tok = strtok(NULL, " ");
                long steps = strtoul(tok, NULL, 10);

                cycles += run_system_and_check(steps, line, linenum++);
            }
            cycles -= SHORTLINE_CYCLES;
            ai_step(SHORTLINE_CYCLES);
        }
        for (; n64sys.vi.v_current < NUM_SHORTLINES + NUM_LONGLINES; n64sys.vi.v_current++) {
            check_vi_interrupt();
            check_vsync();
            while (cycles <= LONGLINE_CYCLES) {
                cycles += run_system_check_interrupt();
                if (cycles > LONGLINE_CYCLES) {
                    break;
                }
                char *line = NULL;
                size_t len = 0;

                if (getline(&line, &len, fp) == -1) {
                    logalways("End of file.");
                    exit(0);
                }

                char* tok = strtok(line, " ");
                tok = strtok(NULL, " ");
                long steps = strtoul(tok, NULL, 10);

                cycles += run_system_and_check(steps, line, linenum++);
            }
            cycles -= LONGLINE_CYCLES;
            ai_step(LONGLINE_CYCLES);
        }
        check_vi_interrupt();
        check_vsync();
    }
}

int main(int argc, char** argv) {
    const char* log_file = NULL;
    const char* rdp_plugin_path = NULL;
    bool test_rsp = false;
    bool test_jit_sync = false;

    cflags_t* flags = cflags_init();
    cflags_flag_t * verbose = cflags_add_bool(flags, 'v', "verbose", NULL, "enables verbose output, repeat up to 4 times for more verbosity");
    cflags_add_string(flags, 'f', "log-file", &log_file, "log file to check run against");
    cflags_add_bool(flags, 's', "rsp", &test_rsp, "check RSP log file instead of CPU log file");
    cflags_add_bool(flags, 'j', "jitsync", &test_jit_sync, "check JIT sync point log file against interpreter instead of CPU log file");
    cflags_add_string(flags, 'r', "rdp", &rdp_plugin_path, "Load RDP plugin (Mupen64Plus compatible)");

    const char* pif_rom_path = NULL;
    cflags_add_string(flags, 'p', "pif", &pif_rom_path, "Load PIF ROM");

    cflags_parse(flags, argc, argv);

    if (flags->argc != 1) {
        usage(flags);
        return 1;
    }

    if (test_rsp && test_jit_sync) {
        logfatal("can't pass both -s and -j");
    }

    if (!log_file) {
        logfatal("Must pass a log file with -f!");
    }

    FILE* fp = fopen(log_file, "r");

    const char* rom = flags->argv[0];

    log_set_verbosity(verbose->count);

    if (rdp_plugin_path != NULL) {
        init_n64system(rom, true, false, OPENGL_VIDEO_TYPE, false);
        load_rdp_plugin(rdp_plugin_path);
    } else {
        init_n64system(rom, true, false, VULKAN_VIDEO_TYPE, false);
        load_parallel_rdp();
    }
    if (pif_rom_path) {
        load_pif_rom(pif_rom_path);
    }
    pif_rom_execute();

    if (test_rsp) {
        check_rsp_log(fp);
    } else if (test_jit_sync) {
        check_jit_sync_log(fp);
    } else {
        check_cpu_log(fp);
    }

    n64_system_cleanup();
}