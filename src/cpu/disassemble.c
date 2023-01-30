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
        logwarn("Failed to disassemble code!");
        snprintf(buf, buflen, "ERROR! big (0x%08X) little (0x%08X)", raw, FAKELITTLE_WORD(raw));
        return 0;
    } else if (count > 1) {
        logwarn("Given more than one instruction, only disassembling the first one!");
    }

    snprintf(buf, buflen, "%s %s", insn[0].mnemonic, insn[0].op_str);

    cs_free(insn, count);
#else
    snprintf(buf, buflen, "[Disassembly Unsupported]");
#endif
    return 1;
}

int disassemble_x86_64(uintptr_t address, u8* code, size_t code_size, char* buf, size_t buflen) {
#ifdef HAVE_CAPSTONE
    disassembler_initialize();
    size_t count = cs_disasm(handle_x86_64, code, code_size, address, 0, &insn);
    for (int i = 0; i < count; i++) {
        int written = snprintf(buf, buflen, "%s %s\n", insn[i].mnemonic, insn[i].op_str);
        buf += written;
        buflen -= written;
    }

    cs_free(insn, count);
#else
    snprintf(buf, buflen, "[Disassembly Unsupported]");
#endif
    return 1;
}