#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include "../src/system/n64system.h"
#include "../src/cpu/rsp.h"

// Just to make sure we don't get caught in an infinite loop
#define MAX_CYCLES 100000

void load_rsp_imem(n64_system_t* system, const char* rsp_path) {
    FILE* rsp = fopen(rsp_path, "rb");
    size_t read = fread(system->mem.sp_imem, 1, SP_IMEM_SIZE, rsp);
    printf("Loaded from %s: %ld bytes.", rsp_path, read);
}

void load_rsp_dmem(n64_system_t* system, word* input, int input_size) {
    for (int i = 0; i < input_size; i++) {
        system->rsp.write_word(i, input[i]);
    }
}

void run_test(const char* test_name, const char* subtest_name, word* input, int input_size, word* output, int output_size) {
    n64_system_t* system = init_n64system(NULL, false);

    char rsp_path[PATH_MAX];
    snprintf(rsp_path, PATH_MAX, "%s.rsp", test_name);

    load_rsp_imem(system, rsp_path);
    load_rsp_dmem(system, input, input_size / 4);

    system->rsp.status.halt = false;

    int cycles = 0;

    while (!system->rsp.status.halt && cycles < MAX_CYCLES) {
        cycles++;
        rsp_step(system);
    }

    free(system);
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

    for (int i = 4; i < argc; i++) {
        const char* subtest_name = argv[i];
        byte input[input_size];
        fread(input, 1, input_size, input_data_handle);
        byte output[output_size];
        fread(output, 1, output_size, output_data_handle);

        run_test(test_name, subtest_name, (word*)input, input_size, (word*)output, output_size);
    }

    exit(0);
}