#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include "../src/system/n64system.h"
#include "../src/cpu/rsp.h"

// Just to make sure we don't get caught in an infinite loop
#define MAX_CYCLES 100000

void load_rsp_imem(n64_system_t* system, const char* rsp_path) {
    FILE* rsp = fopen(rsp_path, "rb");
    // This file is already in big endian
    size_t read = fread(system->mem.sp_imem, 1, SP_IMEM_SIZE, rsp);
    if (read == 0) {
        logfatal("Read 0 bytes from %s", rsp_path)
    }
}

void load_rsp_dmem(n64_system_t* system, word* input, int input_size) {
    for (int i = 0; i < input_size; i++) {
        // This translates to big endian
        word address = i * 4;
        system->rsp.write_word(address, input[i]);
    }

}

bool run_test(const char* test_name, const char* subtest_name, word* input, int input_size, word* output, int output_size) {
    n64_system_t* system = init_n64system(NULL, false);

    char rsp_path[PATH_MAX];
    snprintf(rsp_path, PATH_MAX, "%s.rsp", test_name);

    load_rsp_imem(system, rsp_path);
    load_rsp_dmem(system, input, input_size / 4);

    system->rsp.status.halt = false;
    system->rsp.pc = 0;

    int cycles = 0;

    while (!system->rsp.status.halt) {
        if (cycles >= MAX_CYCLES) {
            logfatal("Test ran too long and was killed! Possible infinite loop?")
        }
        cycles++;
        rsp_step(system);
    }

    bool failed = false;
    printf("\n\n================= Expected =================");
    for (int i = 0; i < output_size; i++) {
        if (i % 16 == 0) {
            printf("\n0x%04X:  ", 0x800 + i);
        } else if (i % 4 == 0) {
            printf(" ");
        }
        printf("%02X", ((byte*)output)[i]);
    }

    printf("\n\n================== Actual ==================");
    for (int i = 0; i < output_size; i++) {
        if (i % 16 == 0) {
            printf("\n0x%04X:  ", 0x800 + i);
        } else if (i % 4 == 0) {
            printf(" ");
        }
        byte actual = system->mem.sp_dmem[0x800 + i];
        byte expected = ((byte*)output)[i];

        if (actual != expected) {
            printf(COLOR_RED);
        }
        printf("%02X", actual);
        if (actual != expected) {
            printf(COLOR_END);
        }
    }
    printf("\n\n");

    for (int i = 0; i < output_size / 4; i++) {
        // File is in big endian, and the read handler converts things back to little for us, so convert it here too.
        word expected = be32toh(output[i]);
        word actual = system->rsp.read_word(0x800 + (i * 4));

        if (actual != expected) {
            failed = true;
        }
    }


    free(system);

    return failed;
}

int main(int argc, char** argv) {
    if (argc < 5) {
        logfatal("Not enough arguments")
    }

    const char* test_name = argv[1];
    int input_size = atoi(argv[2]);
    int output_size = atoi(argv[3]);

    if (input_size % 4 != 0) {
        logfatal("Invalid input size: %d is not divisible by 4.", input_size)
    }

    if (output_size % 4 != 0) {
        logfatal("Invalid output size: %d is not divisible by 4.", output_size)
    }


    char input_data_path[PATH_MAX];
    snprintf(input_data_path, PATH_MAX, "%s.input", test_name);
    FILE* input_data_handle = fopen(input_data_path, "rb");

    char output_data_path[PATH_MAX];
    snprintf(output_data_path, PATH_MAX, "%s.golden", test_name);
    FILE* output_data_handle = fopen(output_data_path, "rb");


    bool failed = false;

    for (int i = 4; i < argc; i++) {
        const char* subtest_name = argv[i];
        byte input[input_size];
        fread(input, 1, input_size, input_data_handle);
        byte output[output_size];
        fread(output, 1, output_size, output_data_handle);

        bool subtest_failed = run_test(test_name, subtest_name, (word*)input, input_size, (word*)output, output_size);
        if (subtest_failed) {
            printf("[%s %s] FAILED\n", test_name, subtest_name);
        } else {
            printf("[%s %s] PASSED\n", test_name, subtest_name);
        }

        failed |= subtest_failed;
    }

    if (failed) {
        logdie("Tests failed!")
    }

    exit(0);
}