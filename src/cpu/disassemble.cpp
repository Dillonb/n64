#include "disassemble.h"

#include <stdbool.h>
#include <stdio.h>

#ifdef HAVE_CAPSTONE
#include <capstone/capstone.h>
#endif

#include "disassemble.h"
#include <log.h>

bool disassembler_initialized = false;
#ifdef HAVE_CAPSTONE
csh handle_mips64;
csh handle_x86_64;
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
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle_x86_64) != CS_ERR_OK) {
        logfatal("Failed to initialize capstone ");
    }
#endif

    disassembler_initialized = true;
}

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

    snprintf(buf, buflen, "%s %s", insn[0].mnemonic, insn[0].op_str);

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
            handle = handle_x86_64;
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
                           insn[i].mnemonic, insn[i].op_str);
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
