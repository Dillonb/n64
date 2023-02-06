#include <system/n64system.h>
#include <mem/mem_util.h>

void assert_eq_u64(const char* name, u64 expected, u64 actual) {
    if (actual != expected) {
        logfatal("Expected %s == %016lX, but was %016lX!", name, expected, actual);
    }
}

void assert_reg_value(u64 expected, int reg) {
    u64 actual = N64CPU.gpr[reg];
    assert_eq_u64(register_names[reg], expected, actual);
}

void load_code(const char* path) {
    memset(n64sys.mem.rdram, 0, N64_RDRAM_SIZE);
    FILE* f = fopen(path, "rb");
    if (!f) {
        logfatal("Could not open code file at %s\n", path);
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(n64sys.mem.rdram, size, 1, f);
    fclose(f);

    for (int i = 0; i < size; i += 4) {
        u32 word = word_from_byte_array(n64sys.mem.rdram, i);
        word = be32toh(word);
        word_to_byte_array(n64sys.mem.rdram, i, word);
    }
}

void test_branch_likely(bool jit) {
    init_n64system(NULL, false, false, UNKNOWN_VIDEO_TYPE, false);
    set_pc_word_r4300i(0x80000000);

    load_code("dynarec_v2_tests/branch_likely.bin");


    // run with interpreter: t0 is now 0, and the JIT can't propagate it as a constant
    log_set_verbosity(LOG_VERBOSITY_TRACE);
    n64_system_step(false);
    assert_reg_value(0, MIPS_REG_T0);
    assert_eq_u64("pc", 0xFFFFFFFF80000004, N64CPU.pc);

    n64_system_step(jit); // start on the bnel instruction

    // Branch was NOT taken.
    assert_eq_u64("pc", 0xFFFFFFFF8000000C, N64CPU.pc);
    // Delay slot was skipped.
    assert_reg_value(0, MIPS_REG_T0);
}

int main(int argc, char** argv) {
    test_branch_likely(false);
    test_branch_likely(true);
}