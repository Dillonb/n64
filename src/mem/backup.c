#include "backup.h"

void sram_write_word(n64_system_t* system, word index, word value) {
    if (index >= N64_SRAM_SIZE - 3) {
        logfatal("Out of range SRAM write! index 0x%08X\n", index);
    }
    word_to_byte_array(system->mem.sram, index, value);
}
word sram_read_word(n64_system_t* system, word index) {
    if (index >= N64_SRAM_SIZE - 3) {
        logfatal("Out of range SRAM read! index 0x%08X\n", index);
    }

    return word_from_byte_array(system->mem.sram, index);
}

void sram_write_byte(n64_system_t* system, word index, byte value) {
    if (index >= N64_SRAM_SIZE) {
        logfatal("Out of range SRAM write! index 0x%08X\n", index);
    }
    system->mem.sram[index] = value;
}
byte sram_read_byte(n64_system_t* system, word index) {
    if (index >= N64_SRAM_SIZE) {
        logfatal("Out of range SRAM read! index 0x%08X\n", index);
    }
    return system->mem.sram[index];
}
