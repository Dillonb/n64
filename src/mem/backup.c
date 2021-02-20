#include "backup.h"
#include <limits.h>

#define SAVE_DATA_DEBOUNCE_FRAMES 60
#define MEMPACK_SIZE 32768

void sram_write_word(n64_system_t* system, word index, word value) {
    assert_is_sram(system->mem.save_type);
    if (index >= system->mem.save_size - 3) {
        logfatal("Out of range SRAM write! index 0x%08X\n", index);
    }
    word_to_byte_array(system->mem.save_data, index, htobe32(value));
    system->mem.save_data_dirty = true;
}

word sram_read_word(n64_system_t* system, word index) {
    assert_is_sram(system->mem.save_type);
    if (index >= system->mem.save_size - 3) {
        logwarn("Out of range SRAM read! index 0x%08X\n", index);
        return 0;
    }

    return be32toh(word_from_byte_array(system->mem.save_data, index));
}

void sram_write_byte(n64_system_t* system, word index, byte value) {
    assert_is_sram(system->mem.save_type);
    if (index >= system->mem.save_size) {
        logfatal("Out of range SRAM write! index 0x%08X\n", index);
    }
    system->mem.save_data[index] = value;
    system->mem.save_data_dirty = true;
}

byte sram_read_byte(n64_system_t* system, word index) {
    assert_is_sram(system->mem.save_type);
    if (index >= system->mem.save_size) {
        logfatal("Out of range SRAM read! index 0x%08X\n", index);
    }
    return system->mem.save_data[index];
}

#define FLASH_COMMAND_EXECUTE          0xD2
#define FLASH_COMMAND_STATUS           0xE1
#define FLASH_COMMAND_SET_ERASE_OFFSET 0x4B
#define FLASH_COMMAND_ERASE            0x78
#define FLASH_COMMAND_SET_WRITE_OFFSET 0xA5
#define FLASH_COMMAND_WRITE            0xB4
#define FLASH_COMMAND_READ             0xF0

void flash_command_execute(n64_system_t* system) {
    logalways("flash_command_execute");
    switch (system->mem.flash.state) {

        case FLASH_STATE_IDLE:
            break;
        case FLASH_STATE_ERASE:
            for (int i = 0; i < 128; i++) {
                system->mem.save_data[system->mem.flash.erase_offset + i] = 0xFF;
            }
            system->mem.save_data_dirty = true;
            logalways("Execute erase at offset 0x%X", system->mem.flash.erase_offset);
            break;
        case FLASH_STATE_WRITE:
            for (int i = 0; i < 128; i++) {
                system->mem.save_data[system->mem.flash.write_offset + i] = system->mem.flash.write_buffer[i];
            }
            system->mem.save_data_dirty = true;
            logalways("Copied write buffer to flash starting at 0x%X", system->mem.flash.write_offset);
            break;
        case FLASH_STATE_READ:
            logfatal("Execute command when flash in read state");
            break;
        case FLASH_STATE_STATUS:
            break;
    }
}

void flash_command_status(n64_system_t* system) {
    logalways("flash_command_status");
    system->mem.flash.state      = FLASH_STATE_STATUS;
    system->mem.flash.status_reg = 0x1111800100C20000;
}

void flash_command_write(n64_system_t* system) {
    logalways("flash_command_write");
    system->mem.flash.state = FLASH_STATE_WRITE;
}

void flash_command_read(n64_system_t* system) {
    logalways("flash_command_read");
    system->mem.flash.state      = FLASH_STATE_READ;
    system->mem.flash.status_reg = 0x11118004F0000000;
}

void flash_command_set_erase_offset(n64_system_t* system, word value) {
    logalways("flash_command_set_erase_offset");
    system->mem.flash.erase_offset = (value & 0xFFFF) << 7;
    logalways("Wrote 0x%08X, set erase offset to 0x%X", value, system->mem.flash.erase_offset);
}

void flash_command_erase(n64_system_t* system) {
    logalways("flash_command_erase");
    system->mem.flash.state      = FLASH_STATE_ERASE;
    system->mem.flash.status_reg = 0x1111800800C20000LL;
}

void flash_command_set_write_offset(n64_system_t* system, word value) {
    logalways("flash_command_set_write_offset");
    system->mem.flash.write_offset = (value & 0xFFFF) << 7;
    system->mem.flash.status_reg   = 0x1111800400C20000LL;
}

void flash_write_word(n64_system_t* system, word index, word value) {
    logalways("Flash write word index %X = 0x%08X", index, value);

    if (index > 0) {
        byte command = value >> 24;
        switch (command) {
            case FLASH_COMMAND_EXECUTE:          flash_command_execute(system); break;
            case FLASH_COMMAND_STATUS:           flash_command_status(system); break;
            case FLASH_COMMAND_SET_ERASE_OFFSET: flash_command_set_erase_offset(system, value); break;
            case FLASH_COMMAND_ERASE:            flash_command_erase(system); break;
            case FLASH_COMMAND_SET_WRITE_OFFSET: flash_command_set_write_offset(system, value); break;
            case FLASH_COMMAND_WRITE:            flash_command_write(system); break;
            case FLASH_COMMAND_READ:             flash_command_read(system); break;

            default: logfatal("Unknown (probably invalid) flash command: %02X", command);
        }
    } else {
        logwarn("Ignoring write to flash status register");
    }
}

word flash_read_word(n64_system_t* system, word index) {
    word value = system->mem.flash.status_reg >> 32;
    logalways("Flash read word index 0x%X = 0x%08X", index, value);
    return value;

}

void flash_write_byte(n64_system_t* system, word index, byte value) {
    switch (system->mem.flash.state) {
        case FLASH_STATE_IDLE: logfatal("Flash write byte in state FLASH_STATE_IDLE");
        case FLASH_STATE_ERASE: logfatal("Flash write byte in state FLASH_STATE_ERASE");
        case FLASH_STATE_WRITE:
            logalways("Flash write byte index 0x%X = 0x%02X", index, value);
            unimplemented(index > 0x7F, "Out of range flash write_byte");
            system->mem.flash.write_buffer[index] = value;
            break;
        case FLASH_STATE_READ: logfatal("Flash write byte in state FLASH_STATE_READ");
        case FLASH_STATE_STATUS: logfatal("Flash write byte in state FLASH_STATE_STATUS");
    }
}

byte flash_read_byte(n64_system_t* system, word index) {
    switch (system->mem.flash.state) {
        case FLASH_STATE_IDLE: logfatal("Flash read byte while in state FLASH_STATE_IDLE");
        case FLASH_STATE_WRITE: logfatal("Flash read byte while in state FLASH_STATE_WRITE");
        case FLASH_STATE_READ: {
            byte value = system->mem.save_data[index];
            //logalways("Flash read byte in state read: index 0x%X = 0x%02X", index, value);
            return value;
        }
        case FLASH_STATE_STATUS: {
            word offset = (7 - (index % 8)) * 8;
            byte value = (system->mem.flash.status_reg >> offset) & 0xFF;
            logalways("Flash read byte in state status: index 0x%X = 0x%02X", index, value);
            return value;
        }
        default: logfatal("Flash read byte while in unknown state");
    }
}

void backup_write_word(n64_system_t* system, word index, word value) {
    unimplemented(system->mem.save_data == NULL, "Accessing cartridge backup when not initialized! Is this game in the game DB?");

    switch (system->mem.save_type) {
        case SAVE_NONE:
            logfatal("Accessing cartridge backup with save type SAVE_NONE");
            break;
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
        case SAVE_EEPROM_256k:
            logfatal("Accessing cartridge backup with save type SAVE_EEPROM");
            break;
        case SAVE_FLASH_1m:
            flash_write_word(system, index, value);
            break;
        case SAVE_SRAM_768k:
            sram_write_word(system, index, value);
            break;
    }
}

word backup_read_word(n64_system_t* system, word index) {
    unimplemented(system->mem.save_data == NULL, "Accessing cartridge backup when not initialized! Is this game in the game DB?");

    switch (system->mem.save_type) {
        case SAVE_NONE:
            logfatal("Accessing cartridge backup with save type SAVE_NONE");
            break;
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
        case SAVE_EEPROM_256k:
            logwarn("Accessing cartridge backup with save type SAVE_EEPROM, returning 0 for a word read");
            return 0;
        case SAVE_FLASH_1m:
            return flash_read_word(system, index);
            logfatal("Accessing cartridge backup with save type SAVE_FLASH");
            break;
        case SAVE_SRAM_768k:
            return sram_read_word(system, index);
            break;
    }
}

void backup_write_byte(n64_system_t* system, word index, byte value) {
    unimplemented(system->mem.save_data == NULL, "Accessing cartridge backup when not initialized! Is this game in the game DB?");

    switch (system->mem.save_type) {
        case SAVE_NONE:
            logfatal("Accessing cartridge backup with save type SAVE_NONE");
            break;
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
        case SAVE_EEPROM_256k:
            logfatal("Accessing cartridge backup with save type SAVE_EEPROM");
            break;
        case SAVE_FLASH_1m:
            flash_write_byte(system, index, value);
            break;
        case SAVE_SRAM_768k:
            sram_write_byte(system, index, value);
            break;
    }
}

byte backup_read_byte(n64_system_t* system, word index) {
    unimplemented(system->mem.save_data == NULL, "Accessing cartridge backup when not initialized! Is this game in the game DB?");

    switch (system->mem.save_type) {
        case SAVE_NONE:
            logfatal("Accessing cartridge backup with save type SAVE_NONE");
            break;
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
        case SAVE_EEPROM_256k:
            logwarn("Accessing cartridge backup with save type SAVE_EEPROM, returning 0 for a byte read");
            return 0;
        case SAVE_FLASH_1m:
            return flash_read_byte(system, index);
            break;
        case SAVE_SRAM_768k:
            return sram_read_byte(system, index);
            break;
    }
}


byte get_initial_value(n64_save_type_t save_type) {
    switch (save_type) {
        case SAVE_NONE:
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
        case SAVE_EEPROM_256k:
        case SAVE_SRAM_768k:
            return 0x00;
        case SAVE_FLASH_1m:
            return 0xFF;
        default:
            logfatal("Unknown save type!\n");
    }
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
        case SAVE_FLASH_1m:
            return 131072;
        case SAVE_SRAM_768k:
            return 98304;
        default:
            logfatal("Unknown save type!\n");
    }
}

void create_empty_file(size_t save_size, const char* save_path, byte initial_value) {
    if (save_size > 0) {
        FILE* f = fopen(save_path, "wb");
        byte* blank = malloc(save_size);
        memset(blank, initial_value, save_size);
        fwrite(blank, 1, save_size, f);
        fclose(f);
        free(blank);
    }
}

byte* load_backup_file(const char *rom_path, const char *suffix, size_t save_size, char *path, byte initial_value) {
    size_t save_path_len = strlen(rom_path) + strlen(suffix);
    byte* save_data;
    if (save_path_len >= PATH_MAX) {
        logfatal("Path too long, not creating save file! Calm down with the directories.");
    } else {
        strcpy(path, rom_path);
        strncat(path, suffix, PATH_MAX);

        FILE* f = fopen(path, "rb");

        if (f == NULL) {
            create_empty_file(save_size, path, initial_value);
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
    byte initial_value = get_initial_value(mem->save_type);
    mem->save_data = load_backup_file(rom_path, ".save", save_size, mem->save_file_path, initial_value);
    mem->save_size = save_size;
}


void init_mempack(n64_mem_t* mem, const char* rom_path) {
    if (mem->mempack_data == NULL) {
        mem->mempack_data = load_backup_file(rom_path, ".mempak", MEMPACK_SIZE, mem->mempack_file_path, 0x00);
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