#include "backup.h"
#include <limits.h>

#define SAVE_DATA_DEBOUNCE_FRAMES 60
#define MEMPACK_SIZE 32768

void sram_write_word(n64_system_t* system, word index, word value) {
    unimplemented(system->mem.save_data == NULL, "Accessing cartridge SRAM when not initialized! Is this game in the game DB?");
    if (index >= system->mem.save_size - 3) {
        logfatal("Out of range SRAM write! index 0x%08X\n", index);
    }
    word_to_byte_array(system->mem.save_data, index, htobe32(value));
    system->mem.save_data_dirty = true;
}
word sram_read_word(n64_system_t* system, word index) {
    if (system->mem.save_data == NULL) {
        logwarn("Accessing cartridge SRAM when not initialized! Is this game in the game DB?");
        return 0;
    }
    if (index >= system->mem.save_size - 3) {
        logwarn("Out of range SRAM read! index 0x%08X\n", index);
        return 0;
    }

    return be32toh(word_from_byte_array(system->mem.save_data, index));
}

void sram_write_byte(n64_system_t* system, word index, byte value) {
    unimplemented(system->mem.save_data == NULL, "Accessing cartridge SRAM when not initialized! Is this game in the game DB?");
    if (index >= system->mem.save_size) {
        logfatal("Out of range SRAM write! index 0x%08X\n", index);
    }
    system->mem.save_data[index] = value;
    system->mem.save_data_dirty = true;
}
byte sram_read_byte(n64_system_t* system, word index) {
    unimplemented(system->mem.save_data == NULL, "Accessing cartridge SRAM when not initialized! Is this game in the game DB?");
    if (index >= system->mem.save_size) {
        logfatal("Out of range SRAM read! index 0x%08X\n", index);
    }
    return system->mem.save_data[index];
}

size_t get_save_size(n64_save_type_t save_type) {
    switch (save_type) {
        case SAVE_NONE:
            return 0;
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

void create_empty_file(size_t save_size, const char* save_path) {
    if (save_size > 0) {
        FILE* f = fopen(save_path, "wb");
        byte* zeroes = malloc(save_size);
        memset(zeroes, 0, save_size);
        fwrite(zeroes, 1, save_size, f);
        fclose(f);
        free(zeroes);
    }
}

byte* load_backup_file(const char *rom_path, const char *suffix, size_t save_size, char *path) {
    size_t save_path_len = strlen(rom_path) + strlen(suffix);
    byte* save_data;
    if (save_path_len >= PATH_MAX) {
        logfatal("Path too long, not creating save file! Calm down with the directories.");
    } else {
        strcpy(path, rom_path);
        strncat(path, suffix, PATH_MAX);

        FILE* f = fopen(path, "rb");

        if (f == NULL) {
            create_empty_file(save_size, path);
            f = fopen(path, "rb");
        }

        fseek(f, 0, SEEK_END);
        size_t actual_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (actual_size != save_size) {
            logfatal("Corrupted save file: wrong size!");
        }

        save_data = malloc(actual_size);
        fread(save_data, actual_size, 1, f);
    }

    return save_data;
}

void init_savedata(n64_mem_t* mem, const char* rom_path) {
    if (mem->save_type == SAVE_NONE) {
        return;
    }

    size_t save_size = get_save_size(mem->save_type);
    mem->save_data = load_backup_file(rom_path, ".save", save_size, mem->save_file_path);
    mem->save_size = save_size;
}


void init_mempack(n64_mem_t* mem, const char* rom_path) {
    if (mem->mempack_data == NULL) {
        mem->mempack_data = load_backup_file(rom_path, ".mempak", MEMPACK_SIZE, mem->mempack_file_path);
    }
}

void persist(bool* dirty, int* debounce_counter, size_t size, const char* file_path, byte* data, const char* name) {
    if (*dirty) {
        *dirty = false;
        *debounce_counter = SAVE_DATA_DEBOUNCE_FRAMES;
    } else if (*debounce_counter >= 0) {
        if ((*debounce_counter)-- == 0) {
            FILE* f = fopen(file_path, "wb");
            fwrite(data, 1, size, f);
            fclose(f);
            logalways("Persisted %s data to disk", name);
        }
    }
}

void persist_backup(n64_system_t* system) {
    persist(&system->mem.save_data_dirty,
            &system->mem.save_data_debounce_counter,
            system->mem.save_size,
            system->mem.save_file_path,
            system->mem.save_data,
            "save");

    persist(&system->mem.mempack_data_dirty,
            &system->mem.mempack_data_debounce_counter,
            MEMPACK_SIZE,
            system->mem.mempack_file_path,
            system->mem.mempack_data,
            "mempack");
}

void force_persist_backup(n64_system_t* system) {
    bool should_persist = false;
    if (system->mem.save_data_dirty || system->mem.save_data_debounce_counter > 0) {
        system->mem.save_data_debounce_counter = 0;
        should_persist = true;
    }

    if (system->mem.mempack_data_dirty || system->mem.mempack_data_debounce_counter > 0) {
        system->mem.mempack_data_debounce_counter = 0;
        should_persist = true;
    }

    if (should_persist) {
        persist_backup(system);
    }
}