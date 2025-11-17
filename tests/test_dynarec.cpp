#include <cpu/dynarec/dynarec.h>
#include <cpu/r4300i.h>
#include <mem/n64bus.h>
#include <memory>
#include <optional>
#include <stdio.h>
#include <util.h>

#include "jit_rs.h"

#include <vector>

class TestCase {
public:
  TestCase(u64 virtual_pc, const std::initializer_list<u32> &instructions)
      : virtual_pc(virtual_pc), mips_instructions(instructions) {}

  u64 get_virtual_pc() const { return virtual_pc; }

  u32 get_physical_pc() const {
    bool cached;
    return resolve_virtual_address_or_die(virtual_pc, BUS_LOAD, &cached);
  }

  u32 *get_instructions() { return mips_instructions.data(); }

  size_t get_instruction_count() const { return mips_instructions.size(); }

  void set_words_read(const std::initializer_list<std::pair<u32, u32>> &words) {
    words_read = words;
  }

  void
  set_halves_read(const std::initializer_list<std::pair<u32, u16>> &halves) {
    halves_read = halves;
  }

  void set_initial_gprs(const std::initializer_list<u64> &gprs) {
    std::vector<u64> gprs_vec = gprs;
    if (gprs_vec.size() != 32) {
      logfatal("Initial GPRs list must have exactly 32 entries.");
    }
    if (gprs_vec[0] != 0) {
      logfatal("GPR 0 must be initialized to 0.");
    }
    for (size_t i = 0; i < 32; i++) {
      N64CPU.gpr[i] = gprs_vec[i];
    }
  }

  void set_expected_gprs(const std::initializer_list<u64> &gprs) {
    expected_gprs = gprs;
    if (expected_gprs.size() != 32) {
      logfatal("Expected GPRs list must have exactly 32 entries.");
    }
  }
  void set_expected_pc(u64 pc) { expected_pc = pc; }

  u8 read_byte(u32 address) {
    if (bytes_read_index >= bytes_read.size()) {
      logfatal("No more bytes to read in test case!");
    }

    auto to_read = bytes_read[bytes_read_index++];
    if (to_read.first != address) {
      logfatal("Expected to read byte from address 0x%08X but got 0x%08X",
               to_read.first, address);
    }

    return to_read.second;
  }
  u16 read_half(u32 address) {
    if (halves_read_index >= halves_read.size()) {
      logfatal("No more halves to read in test case!");
    }

    auto to_read = halves_read[halves_read_index++];
    if (to_read.first != address) {
      logfatal("Expected to read half from address 0x%08X but got 0x%08X",
               to_read.first, address);
    }
    return to_read.second;
  }
  u32 read_word(u32 address) {
    if (words_read_index >= words_read.size()) {
      logfatal("No more words to read in test case!");
    }

    auto to_read = words_read[words_read_index++];
    if (to_read.first != address) {
      logfatal("Expected to read word from address 0x%08X but got 0x%08X",
               to_read.first, address);
    }
    return to_read.second;
  }
  u64 read_dword(u32 address) {
    if (dwords_read_index >= dwords_read.size()) {
      logfatal("No more dwords to read in test case!");
    }

    auto to_read = dwords_read[dwords_read_index++];
    if (to_read.first != address) {
      logfatal("Expected to read dword from address 0x%08X but got 0x%08X",
               to_read.first, address);
    }
    return to_read.second;
  }

  void write_byte(u32 address, u32 value) {
    if (bytes_written_index >= bytes_written.size()) {
      logfatal("No more bytes to write in test case!");
    }

    auto to_write = bytes_written[bytes_written_index++];
    if (to_write.first != address) {
      logfatal("Expected to write byte to address 0x%08X but got 0x%08X",
               to_write.first, address);
    }
    if (to_write.second != value) {
      logfatal("Expected to write byte value 0x%02X but got 0x%02X",
               to_write.second, value);
    }
  }
  void write_half(u32 address, u32 value) {
    if (halves_written_index >= halves_written.size()) {
      logfatal("No more halves to write in test case!");
    }
    auto to_write = halves_written[halves_written_index++];
    if (to_write.first != address) {
      logfatal("Expected to write half to address 0x%08X but got 0x%08X",
               to_write.first, address);
    }
    if (to_write.second != value) {
      logfatal("Expected to write half value 0x%04X but got 0x%04X",
               to_write.second, value);
    }
  }
  void write_word(u32 address, u32 value) {
    if (words_written_index >= words_written.size()) {
      logfatal("No more words to write in test case!");
    }
    auto to_write = words_written[words_written_index++];
    if (to_write.first != address) {
      logfatal("Expected to write word to address 0x%08X but got 0x%08X",
               to_write.first, address);
    }
    if (to_write.second != value) {
      logfatal("Expected to write word value 0x%08X but got 0x%08X",
               to_write.second, value);
    }
  }
  void write_dword(u32 address, u64 value) {
    if (dwords_written_index >= dwords_written.size()) {
      logfatal("No more dwords to write in test case!");
    }
    auto to_write = dwords_written[dwords_written_index++];
    if (to_write.first != address) {
      logfatal("Expected to write dword to address 0x%08X but got 0x%08X",
               to_write.first, address);
    }
    if (to_write.second != value) {
      logfatal("Expected to write dword value 0x%016" PRIX64 " but got 0x%016" PRIX64,
               to_write.second, value);
    }
  }

  void validate() {
    if (bytes_read_index != bytes_read.size()) {
      logfatal("Not all expected byte reads were performed!");
    }
    if (halves_read_index != halves_read.size()) {
      logfatal("Not all expected half reads were performed!");
    }
    if (words_read_index != words_read.size()) {
      logfatal("Not all expected word reads were performed!");
    }
    if (dwords_read_index != dwords_read.size()) {
      logfatal("Not all expected dword reads were performed!");
    }
    if (bytes_written_index != bytes_written.size()) {
      logfatal("Not all expected byte writes were performed!");
    }
    if (halves_written_index != halves_written.size()) {
      logfatal("Not all expected half writes were performed!");
    }
    if (words_written_index != words_written.size()) {
      logfatal("Not all expected word writes were performed!");
    }
    if (dwords_written_index != dwords_written.size()) {
      logfatal("Not all expected dword writes were performed!");
    }
    if (expected_gprs.size() > 0) {
      for (size_t i = 0; i < expected_gprs.size(); i++) {
        if (N64CPU.gpr[i] != expected_gprs[i]) {
          logfatal("GPR %zu: expected 0x%016" PRIX64 " but got 0x%016" PRIX64,
                   i, expected_gprs[i], N64CPU.gpr[i]);
        }
      }
    }
    if (expected_pc.has_value()) {
      if (N64CPU.pc != expected_pc.value()) {
        logfatal("PC: expected 0x%016" PRIX64 " but got 0x%016" PRIX64,
                 expected_pc.value(), N64CPU.pc);
      }
    }
  }

private:
  u64 virtual_pc;

  std::vector<u32> mips_instructions;

  std::vector<u64> expected_gprs;
  std::optional<u64> expected_pc;

  std::vector<std::pair<u32, u8>> bytes_read;
  size_t bytes_read_index = 0;
  std::vector<std::pair<u32, u16>> halves_read;
  size_t halves_read_index = 0;
  std::vector<std::pair<u32, u32>> words_read;
  size_t words_read_index = 0;
  std::vector<std::pair<u32, u64>> dwords_read;
  size_t dwords_read_index = 0;

  std::vector<std::pair<u32, u32>> bytes_written;
  size_t bytes_written_index = 0;
  std::vector<std::pair<u32, u32>> halves_written;
  size_t halves_written_index = 0;
  std::vector<std::pair<u32, u32>> words_written;
  size_t words_written_index = 0;
  std::vector<std::pair<u32, u64>> dwords_written;
  size_t dwords_written_index = 0;
};
std::unique_ptr<TestCase> current_testcase;

u8 mock_read_physical_byte(u32 address) {
    return current_testcase->read_byte(address);
}
u16 mock_read_physical_half(u32 address) {
  return current_testcase->read_half(address);
}
u32 mock_read_physical_word(u32 address) {
  return current_testcase->read_word(address);
}
u64 mock_read_physical_dword(u32 address) {
    return current_testcase->read_dword(address);
}
void mock_write_physical_byte(u32 address, u32 value) {
    return current_testcase->write_byte(address, value);
}
void mock_write_physical_half(u32 address, u32 value) {
    return current_testcase->write_half(address, value);
}
void mock_write_physical_word(u32 address, u32 value) {
    return current_testcase->write_word(address, value);
}
void mock_write_physical_dword(u32 address, u64 value) {
    return current_testcase->write_dword(address, value);
}

int main(int argc, char **argv) {
  // Needed to setup all static global pointers that code assumes exist
  init_n64system(NULL, false, false, UNKNOWN_VIDEO_TYPE, true);

  current_testcase = std::unique_ptr<TestCase>(
      new TestCase(0xFFFFFFFF80320F9C,
                   {
                       0x24E72618, // addiu $a3, $a3, 0x2618
                       0xC4E60020, // lwc1 $f6, 0x20($a3)
                       0x44802000, // mtc1 $zero, $f4
                       0x97AE0022, // lhu $t6, 0x22($sp)
                       0x3C028036, // lui $v0, 0x8036
                       0x46062032, // c.eq.s $f4, $f6
                       0x3C198036, // lui $t9, 0x8036
                       0x3C098033, // lui $t1, 0x8033
                       0x45000005, // bc1f -0x7fcdf02c
                       0x00000000, // nop
                   }));
  current_testcase->set_words_read({
      {0x00222638, 0x00000000},
  });
  current_testcase->set_halves_read({
      {0x00206CE2, 0x0000},
  });
  current_testcase->set_initial_gprs({0,
                                      0x000000000000001A,
                                      0xFFFFFFFF80222618,
                                      0xFFFFFFFF801EE0A0,
                                      0x0000000000000000,
                                      0x00000000000000FF,
                                      0x00000000000000FF,
                                      0xFFFFFFFF80220000,
                                      0x0000000000000000,
                                      0x00000000000000E0,
                                      0xFFFFFFFF80333B94,
                                      0xFFFFFFFF80333BA4,
                                      0x0000000000000003,
                                      0x0000000000000001,
                                      0x0000000000000000,
                                      0xFFFFFFFF80222618,
                                      0x0000000000000000,
                                      0x0000000000000000,
                                      0x0000000000000000,
                                      0x0000000000000000,
                                      0x0000000000000000,
                                      0x0000000000000000,
                                      0x0000000000000000,
                                      0x0000000000000000,
                                      0x0000000076557364,
                                      0xFFFFFFFF80335004,
                                      0xFFFFFFFFA430000C,
                                      0x0000000000000AAA,
                                      0x0000000000000000,
                                      0xFFFFFFFF80206CC0,
                                      0x0000000000000000,
                                      0xFFFFFFFF80320618});

    current_testcase->set_expected_pc(0xFFFFFFFF80320FC4);

  MipsToIrContext context = {
      .read_physical_byte = (uintptr_t)&mock_read_physical_byte,
      .read_physical_half = (uintptr_t)&mock_read_physical_half,
      .read_physical_word = (uintptr_t)&mock_read_physical_word,
      .read_physical_dword = (uintptr_t)&mock_read_physical_dword,
      .write_physical_byte = (uintptr_t)&mock_write_physical_byte,
      .write_physical_half = (uintptr_t)&mock_write_physical_half,
      .write_physical_word = (uintptr_t)&mock_write_physical_word,
      .write_physical_dword = (uintptr_t)&mock_write_physical_dword,
  };

  rs_jit_compile_and_run_block_for_test(
      current_testcase->get_instructions(),
      current_testcase->get_instruction_count(),
      current_testcase->get_virtual_pc(), current_testcase->get_physical_pc(),
      n64cpu_ptr, context);

  current_testcase->validate();
}
