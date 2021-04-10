#include "backup.h"
#include <limits.h>

#define SAVE_DATA_DEBOUNCE_FRAMES 60
#define MEMPACK_SIZE 32768

void sram_write_word(word index, word value) {
    assert_is_sram(n64sys.mem.save_type);
    if (index >= n64sys.mem.save_size - 3) {
        logfatal("Out of range SRAM write! index 0x%08X\n", index);
    }
    word_to_byte_array(n64sys.mem.save_data, index, htobe32(value));
    n64sys.mem.save_data_dirty = true;
}

word sram_read_word(word index) {
    assert_is_sram(n64sys.mem.save_type);
    if (index >= n64sys.mem.save_size - 3) {
        logwarn("Out of range SRAM read! index 0x%08X\n", index);
        return 0;
    }

    return be32toh(word_from_byte_array(n64sys.mem.save_data, index));
}

void sram_write_byte(word index, byte value) {
    assert_is_sram(n64sys.mem.save_type);
    if (index >= n64sys.mem.save_size) {
        logfatal("Out of range SRAM write! index 0x%08X\n", index);
    }
    n64sys.mem.save_data[index] = value;
    n64sys.mem.save_data_dirty = true;
}

byte sram_read_byte(word index) {
    assert_is_sram(n64sys.mem.save_type);
    if (index >= n64sys.mem.save_size) {
        logfatal("Out of range SRAM read! index 0x%08X\n", index);
    }
    return n64sys.mem.save_data[index];
}

#define FLASH_COMMAND_EXECUTE          0xD2
#define FLASH_COMMAND_STATUS           0xE1
#define FLASH_COMMAND_SET_ERASE_OFFSET 0x4B
#define FLASH_COMMAND_ERASE            0x78
#define FLASH_COMMAND_SET_WRITE_OFFSET 0xA5
#define FLASH_COMMAND_WRITE            0xB4
#define FLASH_COMMAND_READ             0xF0

void flash_command_execute() {
    logdebug("flash_command_execute");
    switch (n64sys.mem.flash.state) {

        case FLASH_STATE_IDLE:
            break;
        case FLASH_STATE_ERASE:
            for (int i = 0; i < 128; i++) {
                n64sys.mem.save_data[n64sys.mem.flash.erase_offset + i] = 0xFF;
            }
            n64sys.mem.save_data_dirty = true;
            logdebug("Execute erase at offset 0x%zX", n64sys.mem.flash.erase_offset);
            break;
        case FLASH_STATE_WRITE:
            for (int i = 0; i < 128; i++) {
                n64sys.mem.save_data[n64sys.mem.flash.write_offset + i] = n64sys.mem.flash.write_buffer[i];
            }
            n64sys.mem.save_data_dirty = true;
            logdebug("Copied write buffer to flash starting at 0x%zX", n64sys.mem.flash.write_offset);
            break;
        case FLASH_STATE_READ:
            logfatal("Execute command when flash in read state");
            break;
        case FLASH_STATE_STATUS:
            break;
    }
}

void flash_command_status() {
    logdebug("flash_command_status");
    n64sys.mem.flash.state      = FLASH_STATE_STATUS;
    n64sys.mem.flash.status_reg = 0x1111800100C20000;
}

void flash_command_write() {
    logdebug("flash_command_write");
    n64sys.mem.flash.state = FLASH_STATE_WRITE;
}

void flash_command_read() {
    logdebug("flash_command_read");
    n64sys.mem.flash.state      = FLASH_STATE_READ;
    n64sys.mem.flash.status_reg = 0x11118004F0000000;
}

void flash_command_set_erase_offset(word value) {
    logdebug("flash_command_set_erase_offset");
    n64sys.mem.flash.erase_offset = (value & 0xFFFF) << 7;
    logdebug("Wrote 0x%08X, set erase offset to 0x%zX", value, n64sys.mem.flash.erase_offset);
}

void flash_command_erase() {
    logdebug("flash_command_erase");
    n64sys.mem.flash.state      = FLASH_STATE_ERASE;
    n64sys.mem.flash.status_reg = 0x1111800800C20000LL;
}

void flash_command_set_write_offset(word value) {
    logdebug("flash_command_set_write_offset");
    n64sys.mem.flash.write_offset = (value & 0xFFFF) << 7;
    n64sys.mem.flash.status_reg   = 0x1111800400C20000LL;
}

void flash_write_word(word index, word value) {
    if (index > 0) {
        byte command = value >> 24;
        switch (command) {
            case FLASH_COMMAND_EXECUTE:          flash_command_execute(); break;
            case FLASH_COMMAND_STATUS:           flash_command_status(); break;
            case FLASH_COMMAND_SET_ERASE_OFFSET: flash_command_set_erase_offset(value); break;
            case FLASH_COMMAND_ERASE:            flash_command_erase(); break;
            case FLASH_COMMAND_SET_WRITE_OFFSET: flash_command_set_write_offset(value); break;
            case FLASH_COMMAND_WRITE:            flash_command_write(); break;
            case FLASH_COMMAND_READ:             flash_command_read(); break;

            default: logfatal("Unknown (probably invalid) flash command: %02X", command);
        }
    } else {
        logwarn("Ignoring write of 0x%08X to flash status register", value);
    }
}

word flash_read_word(word index) {
    word value = n64sys.mem.flash.status_reg >> 32;
    logdebug("Flash read word index 0x%X = 0x%08X", index, value);
    return value;

}

void flash_write_byte(word index, byte value) {
    switch (n64sys.mem.flash.state) {
        case FLASH_STATE_IDLE: logfatal("Flash write byte in state FLASH_STATE_IDLE");
        case FLASH_STATE_ERASE: logfatal("Flash write byte in state FLASH_STATE_ERASE");
        case FLASH_STATE_WRITE:
            unimplemented(index > 0x7F, "Out of range flash write_byte");
            n64sys.mem.flash.write_buffer[index] = value;
            break;
        case FLASH_STATE_READ: logfatal("Flash write byte in state FLASH_STATE_READ");
        case FLASH_STATE_STATUS: logfatal("Flash write byte in state FLASH_STATE_STATUS");
    }
}

byte flash_read_byte(word index) {
    switch (n64sys.mem.flash.state) {
        case FLASH_STATE_IDLE: logfatal("Flash read byte while in state FLASH_STATE_IDLE");
        case FLASH_STATE_WRITE: logfatal("Flash read byte while in state FLASH_STATE_WRITE");
        case FLASH_STATE_READ: {
            byte value = n64sys.mem.save_data[index];
            logdebug("Flash read byte in state read: index 0x%X = 0x%02X", index, value);
            return value;
        }
        case FLASH_STATE_STATUS: {
            word offset = (7 - (index % 8)) * 8;
            byte value = (n64sys.mem.flash.status_reg >> offset) & 0xFF;
            logdebug("Flash read byte in state status: index 0x%X = 0x%02X", index, value);
            return value;
        }
        default: logfatal("Flash read byte while in unknown state");
    }
}

void backup_write_word(word index, word value) {
    unimplemented(n64sys.mem.save_data == NULL, "Accessing cartridge backup when not initialized! Is this game in the game DB?");

    switch (n64sys.mem.save_type) {
        case SAVE_NONE:
            logfatal("Accessing cartridge backup with save type SAVE_NONE");
            break;
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
            logfatal("Accessing cartridge backup with save type SAVE_EEPROM");
            break;
        case SAVE_FLASH_1m:
            flash_write_word(index, value);
            break;
        case SAVE_SRAM_256k:
            sram_write_word(index, value);
            break;
    }
}

word backup_read_word(word index) {
    switch (n64sys.mem.save_type) {
        case SAVE_NONE:
            return 0;
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
            logwarn("Accessing cartridge backup with save type SAVE_EEPROM, returning 0 for a word read");
            return 0;
        case SAVE_FLASH_1m:
            return flash_read_word(index);
            logfatal("Accessing cartridge backup with save type SAVE_FLASH");
            break;
        case SAVE_SRAM_256k:
            return sram_read_word(index);
            break;
        default:
            logfatal("Backup read word with unknown save type");
    }
}

void backup_write_byte(word index, byte value) {
    unimplemented(n64sys.mem.save_data == NULL, "Accessing cartridge backup when not initialized! Is this game in the game DB?");

    switch (n64sys.mem.save_type) {
        case SAVE_NONE:
            logfatal("Accessing cartridge backup with save type SAVE_NONE");
            break;
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
            logfatal("Accessing cartridge backup with save type SAVE_EEPROM");
            break;
        case SAVE_FLASH_1m:
            flash_write_byte(index, value);
            break;
        case SAVE_SRAM_256k:
            sram_write_byte(index, value);
            break;
    }
}

byte backup_read_byte(word index) {
    unimplemented(n64sys.mem.save_data == NULL, "Accessing cartridge backup when not initialized! Is this game in the game DB?");

    switch (n64sys.mem.save_type) {
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
            logwarn("Accessing cartridge backup with save type SAVE_EEPROM, returning 0 for a byte read");
            return 0;
        case SAVE_FLASH_1m:
            return flash_read_byte(index);
            break;
        case SAVE_SRAM_256k:
            return sram_read_byte(index);
            break;
        default:
        case SAVE_NONE:
            logfatal("Accessing cartridge backup with unknown save type");
            break;
    }
}


byte get_initial_value(n64_save_type_t save_type) {
    switch (save_type) {
        case SAVE_NONE:
        case SAVE_EEPROM_4k:
        case SAVE_EEPROM_16k:
            return 0x00;
        case SAVE_SRAM_256k:
            return 0xFF;
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
        case SAVE_SRAM_256k:
            return 32768;
        case SAVE_FLASH_1m:
            return 131072;
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

void persist_backup() {
    persist(&n64sys.mem.save_data_dirty,
            &n64sys.mem.save_data_debounce_counter,
            n64sys.mem.save_size,
            n64sys.mem.save_file_path,
            n64sys.mem.save_data,
            "save");

    persist(&n64sys.mem.mempack_data_dirty,
            &n64sys.mem.mempack_data_debounce_counter,
            MEMPACK_SIZE,
            n64sys.mem.mempack_file_path,
            n64sys.mem.mempack_data,
            "mempack");
}

void force_persist_backup() {
    bool should_persist = false;
    if (n64sys.mem.save_data_dirty || n64sys.mem.save_data_debounce_counter > 0) {
        n64sys.mem.save_data_debounce_counter = 0;
        should_persist = true;
    }

    if (n64sys.mem.mempack_data_dirty || n64sys.mem.mempack_data_debounce_counter > 0) {
        n64sys.mem.mempack_data_debounce_counter = 0;
        should_persist = true;
    }

    if (should_persist) {
        persist_backup();
    }
}