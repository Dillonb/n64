#include "backup.h"
#include <limits.h>

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

size_t get_save_size(n64_save_type_t save_type) {
    switch (save_type) {
        case SAVE_NONE:
            return 0;
        case SAVE_MEMPAK:
            return 32768;
        case SAVE_EEPROM_4k:
            return 512;
        case SAVE_EEPROM_16k:
            return 2048;
        case SAVE_EEPROM_256k:
            return 32768;
        case SAVE_EEPROM_1Mb:
            return 131072;
        case SAVE_SRAM_768k:
            return 98304;
        default:
            logfatal("Unknown save type!\n");
    }
}

void create_empty_file(n64_save_type_t save_type, const char* save_path) {
    int save_size = get_save_size(save_type);
    if (save_size > 0) {
        FILE* f = fopen(save_path, "wb");
        byte* zeroes = malloc(save_size);
        memset(zeroes, 0, save_size);
        fwrite(zeroes, 1, save_size, f);
        fclose(f);
        free(zeroes);
    }
}

void init_savedata(n64_mem_t* mem, const char* rom_path) {
    if (mem->save_type == SAVE_NONE) {
        return;
    }

    int save_path_len = strlen(rom_path) + 6; // +6 for ".save" + null terminator
    if (save_path_len >= PATH_MAX) {
        logalways("Path too long, not creating save file!\n");
    } else {
        strcpy(mem->save_file_path, rom_path);
        strncat(mem->save_file_path, ".save", PATH_MAX);

        FILE* f = fopen(mem->save_file_path, "rb");

        if (f == NULL) {
            create_empty_file(mem->save_type, mem->save_file_path);
            f = fopen(mem->save_file_path, "rb");
        }

        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);

        size_t expected_size = get_save_size(mem->save_type);

        if (size != expected_size) {
            logfatal("Corrupted save file: wrong size!");
        }

        mem->save_data = malloc(size);
        fread(mem->save_data, size, 1, f);
    }
}
