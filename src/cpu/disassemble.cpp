#include "disassemble.h"

#include <stdbool.h>
#include <stdio.h>
#include <map>
#include <sstream>

#ifdef HAVE_CAPSTONE
#include <capstone/capstone.h>
#endif

#include "disassemble.h"
#include <log.h>

bool disassembler_initialized = false;
#ifdef HAVE_CAPSTONE
csh handle_mips64;
csh handle_host;
cs_insn* insn;
#endif

void disassembler_initialize() {
    if (disassembler_initialized) {
        return;
    }

#ifdef HAVE_CAPSTONE
    if (cs_open(CS_ARCH_MIPS, CS_MODE_MIPS64, &handle_mips64) != CS_ERR_OK) {
        logfatal("Failed to initialize capstone");
    }
#ifdef N64_ARM64
    if (cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &handle_host) != CS_ERR_OK) {
#else
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle_host) != CS_ERR_OK) {
#endif
        logfatal("Failed to initialize capstone ");
    }
#endif

    disassembler_initialized = true;
}

#ifdef HAVE_CAPSTONE
bool should_fix_op_str(cs_insn& insn) {
    switch (insn.id) {
        case MIPS_INS_MFC0:
            return true;
        default:
            return false;
    }
}

std::map<std::string, std::string> reg_to_cp0 = {
        {"$zero,", "$Index"},        // 0
        {"$at,",   "$Random"},       // 1
        {"$v0,",   "$EntryLo0"},     // 2
        {"$v1,",   "$EntryLo1"},     // 3
        {"$a0,",   "$Context"},      // 4
        {"$a1,",   "$PageMask"},     // 5
        {"$a2,",   "$Wired"},        // 6
        {"$a3,",   "$7"},            // 7
        {"$t0,",   "$BadVAddr"},     // 8
        {"$t1,",   "$Count"},        // 9
        {"$t2,",   "$EntryHi"},      // 10
        {"$t3,",   "$Compare"},      // 11
        {"$t4,",   "$Status"},       // 12
        {"$t5,",   "$Cause"},        // 13
        {"$t6,",   "$EPC"},          // 14
        {"$t7,",   "$PRId"},         // 15
        {"$s0,",   "$Config"},       // 16
        {"$s1,",   "$LLAddr"},       // 17
        {"$s2,",   "$WatchLo"},      // 18
        {"$s3,",   "$WatchHi"},      // 19
        {"$s4,",   "$XContext"},     // 20
        {"$s5,",   "$21"},           // 21
        {"$s6,",   "$22"},           // 22
        {"$s7,",   "$23"},           // 23
        {"$t8,",   "$24"},           // 24
        {"$t9,",   "$25"},           // 25
        {"$k0,",   "$ParityError"},  // 26
        {"$k1,",   "$CacheError"},   // 27
        {"$gp,",   "$TagLo"},        // 28
        {"$sp,",   "$TagHi"},        // 29
        {"$fp,",   "$error_epc"},    // 30
        {"$ra,",    "$31"}           // 31
};

// Yes, I know.
std::string fix_op_str(cs_insn& insn) {
    std::string original = insn.op_str;
    switch (insn.id) {
        case MIPS_INS_MTC0:
        case MIPS_INS_DMTC0:
        case MIPS_INS_MFC0:
        case MIPS_INS_DMFC0: {
            std::stringstream tok(original);

            std::string prefix;
            tok >> prefix;

            std::string badcp0;
            tok >> badcp0;

            std::string goodcp0 = reg_to_cp0[badcp0];

            return prefix + " " + goodcp0;
            break;
        }
        default:
            break;
    }
    return original;
}
#endif


int disassemble(u32 address, u32 raw, char* buf, int buflen) {
#ifdef HAVE_CAPSTONE
    u8 code[4];
    code[0] = raw & 0xFF;
    code[1] = (raw >> 8) & 0xFF;
    code[2] = (raw >> 16) & 0xFF;
    code[3] = (raw >> 24) & 0xFF;
    disassembler_initialize();
    size_t count = cs_disasm(handle_mips64, code, 4, address, 0, &insn);
    if (count == 0) {
        snprintf(buf, buflen, "ERROR! big (0x%08X) little (0x%08X)", raw, FAKELITTLE_WORD(raw));
        return 0;
    }

    snprintf(buf, buflen, "%s %s", insn[0].mnemonic, fix_op_str(insn[0]).c_str());

    cs_free(insn, count);
#else
    snprintf(buf, buflen, "[Disassembly Unsupported]");
#endif
    return 1;
}

std::string disassemble_multi(DisassemblyArch arch, uintptr_t address, u8 *code, size_t code_size) {
#ifdef HAVE_CAPSTONE
    std::string output;
    disassembler_initialize();
    csh handle;
    switch (arch) {
        case DisassemblyArch::HOST:
            handle = handle_host;
            break;
        case DisassemblyArch::GUEST:
            handle = handle_mips64;
            break;
    }

    size_t count = cs_disasm(handle, code, code_size, address, 0, &insn);
    for (int i = 0; i < count; i++) {
        char tmp[100];
        int res;
        if (arch == DisassemblyArch::HOST) {
            res = snprintf(tmp, 100, "%016" PRIX64 "\t%s %s\n", insn[i].address, insn[i].mnemonic, insn[i].op_str);
        } else {
            unimplemented(insn->size != 4, "MIPS instruction was not 4 bytes");
            res = snprintf(tmp, 100, "%08" PRIX64 "\t %02X%02X%02X%02X %s %s\n",
                           insn[i].address,
                           insn[i].bytes[3],
                           insn[i].bytes[2],
                           insn[i].bytes[1],
                           insn[i].bytes[0],
                           insn[i].mnemonic, fix_op_str(insn[i]).c_str());
        }
        output += std::string(tmp);
        if (res < 0) {
            return "Error " + std::to_string(res) + " from snprintf when disassembling:";
        }
    }
    cs_free(insn, count);
    return output;
#else
    return "[Disassembly Unsupported]";
#endif
}

void print_multi_host(uintptr_t address, u8 *code, size_t code_size) {
    printf("%s\n", disassemble_multi(DisassemblyArch::HOST, address, code, code_size).c_str());
}
void print_multi_guest(uintptr_t address, u8 *code, size_t code_size) {
    printf("%s\n", disassemble_multi(DisassemblyArch::GUEST, address, code, code_size).c_str());
}
