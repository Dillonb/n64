#include <frontend/tas_movie.h>
#include "pif.h"
#include "n64bus.h"
#include "backup.h"

void pif_rom_execute_hle() {
    N64CPU.gpr[0] = 0;
    N64CPU.gpr[1] = 0;
    N64CPU.gpr[2] = 0;
    N64CPU.gpr[3] = 0;
    N64CPU.gpr[4] = 0;
    N64CPU.gpr[5] = 0;
    N64CPU.gpr[6] = 0;
    N64CPU.gpr[7] = 0;
    N64CPU.gpr[8] = 0;
    N64CPU.gpr[9] = 0;
    N64CPU.gpr[10] = 0;
    N64CPU.gpr[11] = 0;
    N64CPU.gpr[12] = 0;
    N64CPU.gpr[13] = 0;
    N64CPU.gpr[14] = 0;
    N64CPU.gpr[15] = 0;
    N64CPU.gpr[16] = 0;
    N64CPU.gpr[17] = 0;
    N64CPU.gpr[18] = 0;
    N64CPU.gpr[19] = 0;
    N64CPU.gpr[20] = 0x1;
    N64CPU.gpr[21] = 0;
    N64CPU.gpr[22] = 0x3F;
    N64CPU.gpr[23] = 0;
    N64CPU.gpr[24] = 0;
    N64CPU.gpr[25] = 0;
    N64CPU.gpr[26] = 0;
    N64CPU.gpr[27] = 0;
    N64CPU.gpr[28] = 0;
    N64CPU.gpr[29] = 0xFFFFFFFFA4001FF0;
    N64CPU.gpr[30] = 0;
    N64CPU.gpr[31] = 0;

    //N64CP0.index         = 0;
    N64CP0.random        = 0x0000001F;
    //N64CP0.entry_lo0.raw = 0;
    //N64CP0.entry_lo1.raw = 0;
    //N64CP0.context       = 0;
    //N64CP0.page_mask.raw = 0;
    //N64CP0.wired         = 0;
    //N64CP0.r7            = 0;
    //N64CP0.bad_vaddr     = 0;
    //N64CP0.count         = 0;
    //N64CP0.entry_hi.raw  = 0;
    //N64CP0.compare       = 0;
    N64CP0.status.raw    = 0x34000000;
    //N64CP0.cause.raw     = 0;
    //N64CP0.EPC           = 0;
    N64CP0.PRId          = 0x00000B00;
    N64CP0.config        = 0x7006E463;
    //N64CP0.lladdr        = 0;
    //N64CP0.watch_lo.raw  = 0;
    //N64CP0.watch_hi      = 0;
    //N64CP0.x_context     = 0;
    //N64CP0.r21           = 0;
    //N64CP0.r22           = 0;
    //N64CP0.r23           = 0;
    //N64CP0.r24           = 0;
    //N64CP0.r25           = 0;
    //N64CP0.parity_error  = 0;
    //N64CP0.cache_error   = 0;
    //N64CP0.tag_lo        = 0;
    //N64CP0.tag_hi        = 0;
    //N64CP0.error_epc     = 0;
    //N64CP0.r31           = 0;

    loginfo("CP0 status: ie:  %d", N64CP0.status.ie);
    loginfo("CP0 status: exl: %d", N64CP0.status.exl);
    loginfo("CP0 status: erl: %d", N64CP0.status.erl);
    loginfo("CP0 status: ksu: %d", N64CP0.status.ksu);
    loginfo("CP0 status: ux:  %d", N64CP0.status.ux);
    loginfo("CP0 status: sx:  %d", N64CP0.status.sx);
    loginfo("CP0 status: kx:  %d", N64CP0.status.kx);
    loginfo("CP0 status: im:  %d", N64CP0.status.im);
    loginfo("CP0 status: ds:  %d", N64CP0.status.ds);
    loginfo("CP0 status: re:  %d", N64CP0.status.re);
    loginfo("CP0 status: fr:  %d", N64CP0.status.fr);
    loginfo("CP0 status: rp:  %d", N64CP0.status.rp);
    loginfo("CP0 status: cu0: %d", N64CP0.status.cu0);
    loginfo("CP0 status: cu1: %d", N64CP0.status.cu1);
    loginfo("CP0 status: cu2: %d", N64CP0.status.cu2);
    loginfo("CP0 status: cu3: %d", N64CP0.status.cu3);

    n64_write_word(0x04300004, 0x01010101);

    // Copy the first 0x1000 bytes of the cartridge to 0xA4000000
    memcpy(N64RSP.sp_dmem, n64sys.mem.rom.rom, sizeof(byte) * 0x1000);

    set_pc_word_r4300i(0xA4000040);
}

void pif_rom_execute_lle() {
    set_pc_word_r4300i(0x1FC00000 + SVREGION_KSEG1);
}

void pif_rom_execute() {
    switch (n64sys.mem.rom.cic_type) {
        case CIC_NUS_6101_7102:
            n64_write_word(SREGION_PIF_RAM + 0x24, 0x00043F3F);
            break;
        case CIC_NUS_6102_7101:
            n64_write_word(SREGION_PIF_RAM + 0x24, 0x00003F3F);
            break;
        case CIC_NUS_6103_7103:
            n64_write_word(SREGION_PIF_RAM + 0x24, 0x0000783F);
            break;
        case CIC_NUS_6105_7105:
            n64_write_word(SREGION_PIF_RAM + 0x24, 0x0000913F);
            break;
        case CIC_NUS_6106_7106:
            n64_write_word(SREGION_PIF_RAM + 0x24, 0x0000853F);
            break;
        case UNKNOWN_CIC_TYPE:
            logwarn("Unknown CIC type, not writing seed to PIF RAM! The game may not boot!");
    }

    if (n64sys.mem.rom.pif_rom == NULL) {
        logalways("No PIF rom loaded, HLEing...");
        pif_rom_execute_hle();
    } else {
        logalways("PIF rom loaded, executing it...");
        pif_rom_execute_lle();
    }
}


byte data_crc(const byte* data) {
    byte crc = 0;
    for (int i = 0; i <= 32; i++) {
        for (int j = 7; j >= 0; j--) {
            byte xor_val =((crc & 0x80) != 0) ? 0x85 : 0x00;

            crc <<= 1;
            if (i < 32) {
                if ((data[i] & (1 << j)) != 0) {
                    crc |= 1;
                }
            }

            crc ^= xor_val;
        }
    }

    return crc;
}

#define PIF_COMMAND_CONTROLLER_ID 0x00
#define PIF_COMMAND_READ_BUTTONS  0x01
#define PIF_COMMAND_MEMPACK_READ  0x02
#define PIF_COMMAND_MEMPACK_WRITE 0x03
#define PIF_COMMAND_EEPROM_READ   0x04
#define PIF_COMMAND_EEPROM_WRITE  0x05
#define PIF_COMMAND_RESET         0xFF

void pif_command(sbyte cmdlen, byte reslen, int r_index, int* index, int* channel) {
    byte command = n64sys.mem.pif_ram[(*index)++];
    switch (command) {
        case PIF_COMMAND_RESET:
        case PIF_COMMAND_CONTROLLER_ID: {
            if (*channel < 4) {
                bool plugged_in = n64sys.si.controllers[*channel].plugged_in;
                if (plugged_in) {
                    n64sys.mem.pif_ram[(*index)++] = 0x05;
                    n64sys.mem.pif_ram[(*index)++] = 0x00;
                    n64sys.mem.pif_ram[(*index)++] = 0x01; // Controller pak plugged in.
                } else {
                    n64sys.mem.pif_ram[(*index)++] = 0x05;
                    n64sys.mem.pif_ram[(*index)++] = 0x00;
                    n64sys.mem.pif_ram[(*index)++] = 0x01;
                }
            } else if (*channel == 4) { // EEPROM is on channel 4, and sometimes 5.
                n64sys.mem.pif_ram[(*index)++] = 0x00;
                n64sys.mem.pif_ram[(*index)++] = 0x80;
                n64sys.mem.pif_ram[(*index)++] = 0x00;
            } else {
                logfatal("Controller ID on unknown channel %d", *channel);
            }
            (*channel)++;
            break;
        }
        case PIF_COMMAND_READ_BUTTONS: {
            byte bytes[4];
            if (*channel < 4 && n64sys.si.controllers[*channel].plugged_in) {
                if (tas_movie_loaded()) {
                    // Load inputs from TAS movie
                    n64_controller_t controller = tas_next_inputs();
                    bytes[0] = controller.byte1;
                    bytes[1] = controller.byte2;
                    bytes[2] = controller.joy_x;
                    bytes[3] = controller.joy_y;
                } else {
                    // Load inputs normally
                    bytes[0] = n64sys.si.controllers[*channel].byte1;
                    bytes[1] = n64sys.si.controllers[*channel].byte2;
                    bytes[2] = n64sys.si.controllers[*channel].joy_x;
                    bytes[3] = n64sys.si.controllers[*channel].joy_y;
                }
                n64sys.mem.pif_ram[r_index]   |= 0x00; // Success!
                n64sys.mem.pif_ram[(*index)++] = bytes[0];
                n64sys.mem.pif_ram[(*index)++] = bytes[1];
                n64sys.mem.pif_ram[(*index)++] = bytes[2];
                n64sys.mem.pif_ram[(*index)++] = bytes[3];
            } else {
                n64sys.mem.pif_ram[r_index]   |= 0x80; // Device not present
                n64sys.mem.pif_ram[(*index)++] = 0x00;
                n64sys.mem.pif_ram[(*index)++] = 0x00;
                n64sys.mem.pif_ram[(*index)++] = 0x00;
                n64sys.mem.pif_ram[(*index)++] = 0x00;
            }
            (*channel)++;
            break;
        }
        case PIF_COMMAND_MEMPACK_READ: {
            unimplemented(cmdlen != 3, "Mempack read with cmdlen != 3");
            unimplemented(reslen != 33, "Mempack read with reslen != 33");
            init_mempack(&n64sys.mem, n64sys.rom_path);
            // First two bytes in the command are the offset
            half offset = n64sys.mem.pif_ram[(*index)++] << 8;
            offset |= n64sys.mem.pif_ram[(*index)++];

            // low 5 bits are the CRC
            byte crc = offset & 0x1F;
            // offset must be 32-byte aligned
            offset &= ~0x1F;

            loginfo("mempack read: crc %02X offset: %d", crc, offset);

            // The most significant bit in the address is not used for the mempak.
            // The game will identify whether the connected device is a mempak or something else by writing to
            // 0x8000, then reading from 0x0000. If the device returns the same data, it's a mempak. Otherwise, it's
            // something else, depending on the data it returns.
            offset &= 0x7FE0;

            for (int i = 0; i < 32; i++) {
                n64sys.mem.pif_ram[(*index)++] = n64sys.mem.mempack_data[offset + i];
            }

            // CRC byte
            n64sys.mem.pif_ram[(*index)++] = data_crc(&n64sys.mem.mempack_data[offset]);

            break;
        }
        case PIF_COMMAND_MEMPACK_WRITE: {
            unimplemented(cmdlen != 35, "Mempack write with cmdlen != 35");
            unimplemented(reslen != 1, "Mempack write with reslen != 1");
            init_mempack(&n64sys.mem, n64sys.rom_path);
            // First two bytes in the command are the offset
            half offset = n64sys.mem.pif_ram[(*index)++] << 8;
            offset |= n64sys.mem.pif_ram[(*index)++];

            // low 5 bits are the CRC
            byte crc = offset & 0x1F;
            // offset must be 32-byte aligned
            offset &= ~0x1F;
            loginfo("mempack write: crc %02X offset: %d / 0x%04X", crc, offset, offset);

            // The most significant bit in the address is not used for the mempak.
            // The game will identify whether the connected device is a mempak or something else by writing to
            // 0x8000, then reading from 0x0000. If the device returns the same data, it's a mempak. Otherwise, it's
            // something else, depending on the data it returns.
            offset &= 0x7FE0;

            int data_start_index = *index;
            for (int i = 0; i < 32; i++) {
                n64sys.mem.mempack_data[offset + i] = n64sys.mem.pif_ram[(*index)++];
            }
            n64sys.mem.mempack_data_dirty = true;
            // CRC byte
            n64sys.mem.pif_ram[(*index)++] = data_crc(&n64sys.mem.pif_ram[data_start_index]);
            break;
        }
        case PIF_COMMAND_EEPROM_READ:
            assert_is_eeprom(n64sys.mem.save_type);
            unimplemented(cmdlen != 2, "EEPROM read with cmdlen != 2");
            unimplemented(reslen != 8, "EEPROM read with reslen != 8");
            unimplemented(n64sys.mem.save_data == NULL, "EEPROM read when save data is uninitialized! Is this game in the game DB?");
            if (*channel == 4) {
                byte offset = n64sys.mem.pif_ram[(*index)++];
                if ((offset * 8) >= n64sys.mem.save_size) {
                    logfatal("Out of range EEPROM read! offset: 0x%02X", offset);
                }

                for (int i = 0; i < 8; i++) {
                    n64sys.mem.pif_ram[(*index)++] = n64sys.mem.save_data[(offset * 8) + i];
                }
            } else {
                logfatal("EEPROM read on bad channel %d", *channel);
            }
            break;
        case PIF_COMMAND_EEPROM_WRITE:
            assert_is_eeprom(n64sys.mem.save_type);
            unimplemented(cmdlen != 10, "EEPROM write with cmdlen != 10");
            unimplemented(reslen != 1,  "EEPROM write with reslen != 1");
            unimplemented(n64sys.mem.save_data == NULL, "EEPROM write when save data is uninitialized! Is this game in the game DB?");
            if (*channel == 4) {
                byte offset = n64sys.mem.pif_ram[(*index)++];
                if ((offset * 8) >= n64sys.mem.save_size) {
                    logfatal("Out of range EEPROM write! offset: 0x%02X", offset);
                }

                for (int i = 0; i < 8; i++) {
                    n64sys.mem.save_data[(offset * 8) + i] = n64sys.mem.pif_ram[(*index)++];
                }

                n64sys.mem.pif_ram[(*index)++] = 0; // Error byte, I guess it always succeeds?
            } else {
                logfatal("EEPROM write on bad channel %d", *channel);
            }
            n64sys.mem.save_data_dirty = true;
            break;
        default:
            logfatal("Unknown PIF command: %d", command);
    }
}

void process_pif_command() {
    byte control = n64sys.mem.pif_ram[63];
    if (control == 1) {
        int i = 0;
        int channel = 0;
        while (i < 63) {
            byte t = n64sys.mem.pif_ram[i++] & 0x3F;
            if (t == 0) {
                channel++;
            } else if (t == 0x3F) {
                continue;
            } else if (t == 0x3E) {
                break;
            } else {
                int r_index = i;
                byte r = n64sys.mem.pif_ram[i++] & 0x3F; // TODO: out of bounds access possible on invalid data
                pif_command(t, r, r_index, &i, &channel);
            }
        }
    }
}


void update_button(int controller, n64_button_t button, bool held) {
    switch(button) {
        case N64_BUTTON_A:
            n64sys.si.controllers[controller].a = held;
            break;

        case N64_BUTTON_B:
            n64sys.si.controllers[controller].b = held;
            break;

        case N64_BUTTON_Z:
            n64sys.si.controllers[controller].z = held;
            break;

        case N64_BUTTON_DPAD_UP:
            n64sys.si.controllers[controller].dp_up = held;
            break;

        case N64_BUTTON_DPAD_DOWN:
            n64sys.si.controllers[controller].dp_down = held;
            break;

        case N64_BUTTON_DPAD_LEFT:
            n64sys.si.controllers[controller].dp_left = held;
            break;

        case N64_BUTTON_DPAD_RIGHT:
            n64sys.si.controllers[controller].dp_right = held;
            break;

        case N64_BUTTON_START:
            n64sys.si.controllers[controller].start = held;
            break;

        case N64_BUTTON_L:
            n64sys.si.controllers[controller].l = held;
            break;

        case N64_BUTTON_R:
            n64sys.si.controllers[controller].r = held;
            break;

        case N64_BUTTON_C_UP:
            n64sys.si.controllers[controller].c_up = held;
            break;

        case N64_BUTTON_C_DOWN:
            n64sys.si.controllers[controller].c_down = held;
            break;

        case N64_BUTTON_C_LEFT:
            n64sys.si.controllers[controller].c_left = held;
            break;

        case N64_BUTTON_C_RIGHT:
            n64sys.si.controllers[controller].c_right = held;
            break;
    }
}

void update_joyaxis(int controller, sbyte x, sbyte y) {
    n64sys.si.controllers[controller].joy_x = x;
    n64sys.si.controllers[controller].joy_y = y;
}

void update_joyaxis_x(int controller, sbyte x) {
    n64sys.si.controllers[controller].joy_x = x;
}

void update_joyaxis_y(int controller, sbyte y) {
    n64sys.si.controllers[controller].joy_y = y;
}

void load_pif_rom(const char* pif_rom_path) {
    FILE *fp = fopen(pif_rom_path, "rb");

    if (fp == NULL) {
        logfatal("Error opening the PIF ROM file! Are you sure this is a correct path?");
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);

    fseek(fp, 0, SEEK_SET);
    byte *buf = malloc(size);
    fread(buf, size, 1, fp);

    n64sys.mem.rom.pif_rom = buf;
    n64sys.mem.rom.pif_rom_size = size;
}
