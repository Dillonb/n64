#include "debugger.h"
#include "../mem/n64bus.h"

#define GDBSTUB_IMPLEMENTATION
#define GDBSTUB_DEBUG
#include <gdbstub.h>

const char* target_xml =
        "<?xml version=\"1.0\"?>"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<target version=\"1.0\">"
        "<architecture>mips:4300</architecture>"
        "<osabi>none</osabi>"
        "<feature name=\"org.gnu.gdb.mips.cpu\">"
        "        <reg name=\"r0\" bitsize=\"64\" regnum=\"0\"/>"
        "        <reg name=\"r1\" bitsize=\"64\"/>"
        "        <reg name=\"r2\" bitsize=\"64\"/>"
        "        <reg name=\"r3\" bitsize=\"64\"/>"
        "        <reg name=\"r4\" bitsize=\"64\"/>"
        "        <reg name=\"r5\" bitsize=\"64\"/>"
        "        <reg name=\"r6\" bitsize=\"64\"/>"
        "        <reg name=\"r7\" bitsize=\"64\"/>"
        "        <reg name=\"r8\" bitsize=\"64\"/>"
        "        <reg name=\"r9\" bitsize=\"64\"/>"
        "        <reg name=\"r10\" bitsize=\"64\"/>"
        "        <reg name=\"r11\" bitsize=\"64\"/>"
        "        <reg name=\"r12\" bitsize=\"64\"/>"
        "        <reg name=\"r13\" bitsize=\"64\"/>"
        "        <reg name=\"r14\" bitsize=\"64\"/>"
        "        <reg name=\"r15\" bitsize=\"64\"/>"
        "        <reg name=\"r16\" bitsize=\"64\"/>"
        "        <reg name=\"r17\" bitsize=\"64\"/>"
        "        <reg name=\"r18\" bitsize=\"64\"/>"
        "        <reg name=\"r19\" bitsize=\"64\"/>"
        "        <reg name=\"r20\" bitsize=\"64\"/>"
        "        <reg name=\"r21\" bitsize=\"64\"/>"
        "        <reg name=\"r22\" bitsize=\"64\"/>"
        "        <reg name=\"r23\" bitsize=\"64\"/>"
        "        <reg name=\"r24\" bitsize=\"64\"/>"
        "        <reg name=\"r25\" bitsize=\"64\"/>"
        "        <reg name=\"r26\" bitsize=\"64\"/>"
        "        <reg name=\"r27\" bitsize=\"64\"/>"
        "        <reg name=\"r28\" bitsize=\"64\"/>"
        "        <reg name=\"r29\" bitsize=\"64\"/>"
        "        <reg name=\"r30\" bitsize=\"64\"/>"
        "        <reg name=\"r31\" bitsize=\"64\"/>"
        "        <reg name=\"lo\" bitsize=\"64\" regnum=\"33\"/>"
        "        <reg name=\"hi\" bitsize=\"64\" regnum=\"34\"/>"
        "        <reg name=\"pc\" bitsize=\"32\" regnum=\"37\"/>"
        "        </feature>"
        "<feature name=\"org.gnu.gdb.mips.cp0\">"
        "        <reg name=\"status\" bitsize=\"32\" regnum=\"32\"/>"
        "        <reg name=\"badvaddr\" bitsize=\"32\" regnum=\"35\"/>"
        "        <reg name=\"cause\" bitsize=\"32\" regnum=\"36\"/>"
        "        </feature>"
        "<!-- TODO fix the sizes here. How do we deal with configurable sizes? -->"
        "<feature name=\"org.gnu.gdb.mips.fpu\">"
        "        <reg name=\"f0\" bitsize=\"32\" type=\"ieee_single\" regnum=\"38\"/>"
        "        <reg name=\"f1\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f2\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f3\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f4\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f5\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f6\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f7\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f8\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f9\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f10\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f11\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f12\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f13\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f14\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f15\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f16\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f17\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f18\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f19\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f20\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f21\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f22\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f23\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f24\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f25\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f26\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f27\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f28\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f29\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f30\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f31\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"fcsr\" bitsize=\"32\" group=\"float\"/>"
        "        <reg name=\"fir\" bitsize=\"32\" group=\"float\"/>"
        "        </feature>"
        "</target>";

const char* memory_map =
"<?xml version=\"1.0\"?>"
"<memory-map>"
    "<!-- Everything here is described as RAM, because we don't really have any better option. -->"
    "<!-- Main memory bloc: let's go with 8MB straight off the bat. -->"
    "<memory type=\"ram\" start=\"0x00000000\" length=\"0x800000\"/>"
    "<memory type=\"ram\" start=\"0x80000000\" length=\"0x800000\"/>"
    "<memory type=\"ram\" start=\"0xa0000000\" length=\"0x800000\"/>"
    "<!-- EXP1 can go up to 8MB too. -->"
    "<memory type=\"ram\" start=\"0x1f000000\" length=\"0x800000\"/>"
    "<memory type=\"ram\" start=\"0x9f000000\" length=\"0x800000\"/>"
    "<memory type=\"ram\" start=\"0xbf000000\" length=\"0x800000\"/>"
    "<!-- Scratchpad -->"
    "<memory type=\"ram\" start=\"0x1f800000\" length=\"0x400\"/>"
    "<memory type=\"ram\" start=\"0x9f800000\" length=\"0x400\"/>"
    "<!-- Hardware registers -->"
    "<memory type=\"ram\" start=\"0x1f801000\" length=\"0x2000\"/>"
    "<memory type=\"ram\" start=\"0x9f801000\" length=\"0x2000\"/>"
    "<memory type=\"ram\" start=\"0xbf801000\" length=\"0x2000\"/>"
    "<!-- DTL BIOS SRAM -->"
    "<memory type=\"ram\" start=\"0x1fa00000\" length=\"0x200000\"/>"
    "<memory type=\"ram\" start=\"0x9fa00000\" length=\"0x200000\"/>"
    "<memory type=\"ram\" start=\"0xbfa00000\" length=\"0x200000\"/>"
    "<!-- BIOS -->"
    "<memory type=\"ram\" start=\"0x1fc00000\" length=\"0x80000\"/>"
    "<memory type=\"ram\" start=\"0x9fc00000\" length=\"0x80000\"/>"
    "<memory type=\"ram\" start=\"0xbfc00000\" length=\"0x80000\"/>"
    "<!-- This really is only for 0xfffe0130 -->"
    "<memory type=\"ram\" start=\"0xfffe0000\" length=\"0x200\"/>"
"</memory-map>";

void n64_debug_start(n64_system_t* system) {
    system->debugger_state.broken = false;
}

void n64_debug_stop(n64_system_t* system) {
    system->debugger_state.broken = true;
}

void n64_debug_step(n64_system_t* system) {
    system->debugger_state.steps = 1;
    logfatal("Debug step")
}

void n64_debug_set_breakpoint(n64_system_t* system, word address) {
    logfatal("Debug set breakpoint")
}

void n64_debug_clear_breakpoint(n64_system_t* system, word address) {
    logfatal("Debug clear breakpoint")
}

ssize_t n64_debug_get_memory(n64_system_t* system, char* buffer, size_t length, word address, size_t bytes) {
    printf("Get memory: %d bytes from 0x%08X\n", bytes, address);
    int printed = 0;
    for (int i = 0; i < bytes; i++) {
        byte value = n64_read_byte(system, address + i);
        printf("gdb: Reading one byte from 0x%08X: 0x%02X\n", address, value);
        printed += snprintf(buffer + (i*2), length, "%02X", value);
    }
    return printed;
}

ssize_t n64_debug_get_register_value(n64_system_t* system, char * buffer, size_t buffer_length, int reg) {
    switch (reg) {
        case 0 ... 31:
            return snprintf(buffer, buffer_length, "%016lx", system->cpu.gpr[reg]);
        case 32:
            return snprintf(buffer, buffer_length, "%08x", system->cpu.cp0.status.raw);
        case 33:
            return snprintf(buffer, buffer_length, "%016lx", system->cpu.mult_lo);
        case 34:
            return snprintf(buffer, buffer_length, "%016lx", system->cpu.mult_hi);
        case 35:
            return snprintf(buffer, buffer_length, "%08x", system->cpu.cp0.bad_vaddr);
        case 36:
            return snprintf(buffer, buffer_length, "%08x", system->cpu.cp0.cause.raw);
        case 37:
            return snprintf(buffer, buffer_length, "%08x", system->cpu.pc);
        case 38 ... 71: // TODO FPU stuff
            return snprintf(buffer, buffer_length, "%08x", 0);
        default:
            logfatal("debug get register %d value", reg)
    }
}

ssize_t n64_debug_get_general_registers(n64_system_t* system, char * buffer, size_t buffer_length) {
    ssize_t printed = 0;
    for (int i = 0; i < 32; i++) {
        int ofs = i * 16; // 64 bit regs take up 16 ascii chars to print in hex
        if (ofs + 16 > buffer_length) {
            logfatal("Too big!")
        }
        dword reg = htobe64(system->cpu.gpr[i]);
        printed += snprintf(buffer + ofs, buffer_length - ofs, "%016lx", reg);
    }
    return printed;
}

void debugger_init(n64_system_t* system) {
    gdbstub_config_t config;
    config.port                  = 1337;
    config.user_data             = system;
    config.start                 = (gdbstub_start_t) n64_debug_start;
    config.stop                  = (gdbstub_stop_t) n64_debug_stop;
    config.step                  = (gdbstub_step_t) n64_debug_step;
    config.set_breakpoint        = (gdbstub_set_breakpoint_t) n64_debug_set_breakpoint;
    config.clear_breakpoint      = (gdbstub_clear_breakpoint_t) n64_debug_clear_breakpoint;
    config.get_memory            = (gdbstub_get_memory_t) n64_debug_get_memory;
    config.get_register_value    = (gdbstub_get_register_value_t) n64_debug_get_register_value;
    config.get_general_registers = (gdbstub_get_general_registers_t) n64_debug_get_general_registers;

    config.target_config = target_xml;
    config.target_config_length = strlen(target_xml);

    printf("Sizeof target: %ld\n", config.target_config_length);

    config.memory_map = memory_map;
    config.memory_map_length = strlen(memory_map);

    printf("Sizeof memory map: %ld\n", config.memory_map_length);

    system->debugger_state.gdb = gdbstub_init(config);
    if (!system->debugger_state.gdb) {
        logfatal("Failed to initialize GDB stub!")
    }
}

void debugger_tick(n64_system_t* system) {
    gdbstub_tick(system->debugger_state.gdb);
}

void debugger_cleanup(n64_system_t* system) {
    gdbstub_term(system->debugger_state.gdb);
}
