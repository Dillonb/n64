#include <string.h>
#include <log.h>
#include <r4300i.h>
#include <mips_instructions.h>
#include <assert.h>

typedef enum mipsinstr_type_t {
    IMM32
} mipsinstr_type;

#define MATCH(name, handler, type) do { if (strcmp(name, instruction_name) == 0) {instruction_handler = handler; instruction_type = type; } } while(0)

void gen_imm32(char* name, mipsinstr_handler_t handler) {
    const int num_64bit_args = 50000;
    const int num_32bit_args = 50000;

    int num_cases = num_32bit_args + num_64bit_args;

    dword regargs[num_32bit_args + num_64bit_args];
    half immargs[num_32bit_args + num_64bit_args];
    dword expected_result[num_32bit_args + num_64bit_args];

    static_assert(RAND_MAX == 2147483647, "this code depends on RAND_MAX being int32_max");

    for (int i = 0; i < num_32bit_args; i++) {
        sword regarg = rand() << 1;
        half immarg = rand();
        regargs[i] = (sdword)regarg;
        immargs[i] = immarg;

    }

    for (int i = 0; i < num_64bit_args; i++) {
        dword regarg = rand();
        regarg <<= 32;
        regarg |= rand();
        half immarg = rand();
        regargs[num_32bit_args + i] = regarg;
        immargs[num_32bit_args + i] = immarg;
    }
    r4300i_t cpu;

    for (int i = 0; i < num_cases; i++) {
        mips_instruction_t instruction;
        instruction.i.immediate = immargs[i];

        instruction.i.rt = 1;
        instruction.i.rs = 1;
        cpu.gpr[1] = regargs[i];

        handler(&cpu, instruction);

        expected_result[i] = cpu.gpr[1];
    }

    printf("align(4)\n");
    printf("NumCases: \n\tdd $%08X\n\n", num_cases);
    printf("align(8)\n");
    printf("RegArgs:\n");
    for (int i = 0; i < num_cases; i++) {
        printf("\tdd $%016lX\n", regargs[i]);
    }
    printf("align(8)\n");
    printf("Expected:\n");
    for (int i = 0; i < num_cases; i++) {
        printf("\tdd $%016lX\n", expected_result[i]);
    }
    printf("align(2)\n");
    printf("\nImmArgs:\n");
    for (int i = 0; i < num_cases; i++) {
        printf("\tdh $%04X\n", immargs[i]);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        logdie("Usage: %s <instruction>", argv[0]);
    }

    char* instruction_name = argv[1];
    mipsinstr_handler_t instruction_handler = NULL;
    mipsinstr_type instruction_type;

    MATCH("addiu", mips_addiu, IMM32);


    if (instruction_handler == NULL) {
        logdie("unknown/unsupported instruction: %s", instruction_name);
    }

    switch (instruction_type) {
        case IMM32:
            gen_imm32(instruction_name, instruction_handler);
    }
}