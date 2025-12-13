#include <cpu/disassemble.h>
#include <cpu/dynarec/dynarec.h>
#include <cpu/r4300i.h>
#include <mem/n64bus.h>
#include <memory>
#include <optional>
#include <stdio.h>
#include <toml.hpp>
#include <util.h>

#include "jit_rs.h"

#include <vector>

class TestCase {
public:
  TestCase(u64 virtual_pc, const std::vector<u32>& instructions)
      : virtual_pc(virtual_pc), mips_instructions(instructions) {}

  u64 get_virtual_pc() const { return virtual_pc; }

  u32 get_physical_pc() const {
    bool cached;
    return resolve_virtual_address_or_die(virtual_pc, BUS_LOAD, &cached);
  }

  u32 *get_instructions() { return mips_instructions.data(); }

  size_t get_instruction_count() const { return mips_instructions.size(); }

  void set_bytes_read(const std::vector<std::pair<u32, u8>>& bytes) {
    bytes_read = bytes;
  }

  void set_halves_read(const std::vector<std::pair<u32, u16>>& halves) {
    halves_read = halves;
  }

  void set_words_read(const std::vector<std::pair<u32, u32>>& words) {
    words_read = words;
  }

  void set_dwords_read(const std::vector<std::pair<u32, u64>>& dwords) {
    dwords_read = dwords;
  }

  void set_bytes_written(const std::vector<std::pair<u32, u32>>& bytes) {
    bytes_written = bytes;
  }

  void set_halves_written(const std::vector<std::pair<u32, u32>>& halves) {
    halves_written = halves;
  }

  void set_words_written(const std::vector<std::pair<u32, u32>>& words) {
    words_written = words;
  }

  void set_dwords_written(const std::vector<std::pair<u32, u64>>& dwords) {
    dwords_written = dwords;
  }

  void set_initial_gprs(const std::vector<u64> &gprs) {
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

  void set_expected_gprs(const std::vector<u64> &gprs) {
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
      logfatal("Expected to read half from address 0x%08X but got 0x%08X", to_read.first, address);
    }
    return to_read.second;
  }
  u32 read_word(u32 address) {
    if (words_read_index >= words_read.size()) {
      logfatal("No more words to read in test case!");
    }

    auto to_read = words_read[words_read_index++];
    if (to_read.first != address) {
      logfatal("Expected to read word from address 0x%08X but got 0x%08X", to_read.first, address);
    }
    return to_read.second;
  }
  u64 read_dword(u32 address) {
    if (dwords_read_index >= dwords_read.size()) {
      logfatal("No more dwords to read in test case!");
    }

    auto to_read = dwords_read[dwords_read_index++];
    if (to_read.first != address) {
      logfatal("Expected to read dword from address 0x%08X but got 0x%08X", to_read.first, address);
    }
    return to_read.second;
  }

  void write_byte(u32 address, u32 value) {
    if (bytes_written_index >= bytes_written.size()) {
      logfatal("No more bytes to write in test case!");
    }

    auto to_write = bytes_written[bytes_written_index++];
    if (to_write.first != address) {
      logfatal("Expected to write byte to address 0x%08X but got 0x%08X", to_write.first, address);
    }
    if (to_write.second != value) {
      logfatal("Expected to write byte value 0x%02X but got 0x%02X", to_write.second, value);
    }
  }
  void write_half(u32 address, u32 value) {
    if (halves_written_index >= halves_written.size()) {
      logfatal("No more halves to write in test case!");
    }
    auto to_write = halves_written[halves_written_index++];
    if (to_write.first != address) {
      logfatal("Expected to write half to address 0x%08X but got 0x%08X", to_write.first, address);
    }
    if (to_write.second != value) {
      logfatal("Expected to write half value 0x%04X but got 0x%04X", to_write.second, value);
    }
  }
  void write_word(u32 address, u32 value) {
    if (words_written_index >= words_written.size()) {
      logfatal("No more words to write in test case!");
    }
    auto to_write = words_written[words_written_index++];
    if (to_write.first != address) {
      logfatal("Expected to write word to address 0x%08X but got 0x%08X", to_write.first, address);
    }
    if (to_write.second != value) {
      logfatal("Expected to write word value 0x%08X but got 0x%08X", to_write.second, value);
    }
  }
  void write_dword(u32 address, u64 value) {
    if (dwords_written_index >= dwords_written.size()) {
      logfatal("No more dwords to write in test case!");
    }
    auto to_write = dwords_written[dwords_written_index++];
    if (to_write.first != address) {
      logfatal("Expected to write dword to address 0x%08X but got 0x%08X", to_write.first, address);
    }
    if (to_write.second != value) {
      logfatal("Expected to write dword value 0x%016" PRIX64 " but got 0x%016" PRIX64, to_write.second, value);
    }
  }

  void dump_disassembly() {
    print_multi_guest((uintptr_t)get_virtual_pc(), (u8*)get_instructions(), get_instruction_count() * 4);
  }

  void validate() {
    bool bad = false;
    if (bytes_read_index != bytes_read.size()) {
      logalways("Not all expected byte reads were performed!");
      bad = true;
    }
    if (halves_read_index != halves_read.size()) {
      logalways("Not all expected half reads were performed!");
      bad = true;
    }
    if (words_read_index != words_read.size()) {
      logalways("Not all expected word reads were performed!");
      bad = true;
    }
    if (dwords_read_index != dwords_read.size()) {
      logalways("Not all expected dword reads were performed!");
      bad = true;
    }
    if (bytes_written_index != bytes_written.size()) {
      logalways("Not all expected byte writes were performed!");
      bad = true;
    }
    if (halves_written_index != halves_written.size()) {
      logalways("Not all expected half writes were performed!");
      bad = true;
    }
    if (words_written_index != words_written.size()) {
      logalways("Not all expected word writes were performed!");
      bad = true;
    }
    if (dwords_written_index != dwords_written.size()) {
      logalways("Not all expected dword writes were performed!");
      bad = true;
    }
    if (expected_gprs.size() > 0) {
      for (size_t i = 0; i < expected_gprs.size(); i++) {
        if (N64CPU.gpr[i] != expected_gprs[i]) {
          logalways("GPR %zu: expected 0x%016" PRIX64 " but got 0x%016" PRIX64, i, expected_gprs[i], N64CPU.gpr[i]);
          bad = true;
        }
      }
    }
    if (expected_pc.has_value()) {
      if (N64CPU.pc != expected_pc.value()) {
        logalways("PC: expected 0x%016" PRIX64 " but got 0x%016" PRIX64, expected_pc.value(), N64CPU.pc);
        bad = true;
      }
    }
    if (bad) {
      printf("================= Disassembly ================\n");
      dump_disassembly();
      printf("==============================================\n");
      logfatal("Test case failed!");
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
  cd_to_current_exe();
  // Needed to setup all static global pointers that code assumes exist
  init_n64system(NULL, false, false, UNKNOWN_VIDEO_TYPE, true);

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

  auto testcases = toml::parse_file("testcases.toml");

  int testcase_num = 0;
  for (auto&& testcase : *testcases["testcases"].as_array()) {
    printf("Running test %d...\n", ++testcase_num);
    auto table = *testcase.as_table();

    u64 initial_pc = std::strtoull(table["initial_pc"].as_string()->get().c_str(), nullptr, 16);
    u64 expected_pc = std::strtoull(table["expected_pc"].as_string()->get().c_str(), nullptr, 16);

    std::vector<u32> instructions;
    for (auto&& instr : *table["code"].as_array()) {
      instructions.push_back(static_cast<u32>(instr.as_integer()->get()));
    }

    std::vector<u64> initial_gprs;
    for (auto&& gpr : *table["initial_gprs"].as_array()) {
      initial_gprs.push_back(std::strtoull(gpr.as_string()->get().c_str(), nullptr, 16));
    }

    std::vector<u64> expected_gprs;
    for (auto&& gpr : *table["expected_gprs"].as_array()) {
      expected_gprs.push_back(std::strtoull(gpr.as_string()->get().c_str(), nullptr, 16));
    }

    std::vector<std::pair<u32, u8>> bytes_read;
    if (table.contains("bytes_read")) {
      for (auto&& entry : *table["bytes_read"].as_array()) {
        auto pair = *entry.as_table();
        u32 address = pair["address"].as_integer()->get();
        u8 value = pair["value"].as_integer()->get();
        bytes_read.push_back({address, value});
      }
    }

    std::vector<std::pair<u32, u16>> halves_read;
    if (table.contains("halves_read")) {
      for (auto&& entry : *table["halves_read"].as_array()) {
        auto pair = *entry.as_table();
        u32 address = pair["address"].as_integer()->get();
        u16 value = pair["value"].as_integer()->get();
        halves_read.push_back({address, value});
      }
    }

    std::vector<std::pair<u32, u32>> words_read;
    if (table.contains("words_read")) {
      for (auto&& entry : *table["words_read"].as_array()) {
        auto pair = *entry.as_table();
        u32 address = pair["address"].as_integer()->get();
        u32 value = pair["value"].as_integer()->get();
        words_read.push_back({address, value});
      }
    }

    std::vector<std::pair<u32, u64>> dwords_read;
    if (table.contains("dwords_read")) {
      for (auto&& entry : *table["dwords_read"].as_array()) {
        auto pair = *entry.as_table();
        u32 address = pair["address"].as_integer()->get();
        u64 value = std::strtoull(pair["value"].as_string()->get().c_str(), nullptr, 16);
        dwords_read.push_back({address, value});
      }
    }

    current_testcase = std::unique_ptr<TestCase>(new TestCase(initial_pc, instructions));
    current_testcase->set_expected_pc(expected_pc);
    if (!initial_gprs.empty()) {
      current_testcase->set_initial_gprs(initial_gprs);
    }

    if (!expected_gprs.empty()) {
      current_testcase->set_expected_gprs(expected_gprs);
    }

    if (!bytes_read.empty()) {
      current_testcase->set_bytes_read(bytes_read);
    }
    if (!halves_read.empty()) {
      current_testcase->set_halves_read(halves_read);
    }
    if (!words_read.empty()) {
      current_testcase->set_words_read(words_read);
    }
    if (!dwords_read.empty()) {
      current_testcase->set_dwords_read(dwords_read);
    }

    rs_jit_compile_and_run_block_for_test(
        current_testcase->get_instructions(),
        current_testcase->get_instruction_count(),
        current_testcase->get_virtual_pc(), current_testcase->get_physical_pc(),
        n64cpu_ptr, context);

    current_testcase->validate();
    printf("\tOK!\n");
  }
}
