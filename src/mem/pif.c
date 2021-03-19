#include <frontend/tas_movie.h>
#include <frontend/gamepad.h>
#include <rsp.h>
#include "pif.h"
#include "n64bus.h"
#include "backup.h"

#define PIF_COMMAND_CONTROLLER_ID 0x00
#define PIF_COMMAND_READ_BUTTONS  0x01
#define PIF_COMMAND_MEMPACK_READ  0x02
#define PIF_COMMAND_MEMPACK_WRITE 0x03
#define PIF_COMMAND_EEPROM_READ   0x04
#define PIF_COMMAND_EEPROM_WRITE  0x05
#define PIF_COMMAND_RESET         0xFF

#define CMD_CMDLEN_INDEX 0
#define CMD_RESLEN_INDEX 1
#define CMD_COMMAND_INDEX 2
#define CMD_START_INDEX 3

#define CMD_DATA (&cmd[CMD_START_INDEX])

static int pif_channel = 0;

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

    n64_write_physical_word(0x04300004, 0x01010101);

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
            n64_write_physical_word(SREGION_PIF_RAM + 0x24, 0x00043F3F);
            n64_write_physical_word(SREGION_RDRAM + 0x318, N64_RDRAM_SIZE);
            break;
        case CIC_NUS_6102_7101:
            n64_write_physical_word(SREGION_PIF_RAM + 0x24, 0x00003F3F);
            n64_write_physical_word(SREGION_RDRAM + 0x318, N64_RDRAM_SIZE);
            break;
        case CIC_NUS_6103_7103:
            n64_write_physical_word(SREGION_PIF_RAM + 0x24, 0x0000783F);
            break;
        case CIC_NUS_6105_7105:
            n64_write_physical_word(SREGION_PIF_RAM + 0x24, 0x0000913F);
            n64_write_physical_word(SREGION_RDRAM + 0x3F0, N64_RDRAM_SIZE);
            break;
        case CIC_NUS_6106_7106:
            n64_write_physical_word(SREGION_PIF_RAM + 0x24, 0x0000853F);
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

INLINE void pif_controller_reset(byte* cmd, byte* res) {

}

INLINE void pif_controller_id(byte* cmd, byte* res) {
    device_id_for_pif(pif_channel, res);
    pif_channel++;
}

INLINE void pif_read_buttons(byte* cmd, byte* res) {
    if (device_read_buttons_for_pif(pif_channel, res)) {
        cmd[CMD_RESLEN_INDEX] |= 0x00; // Success!
    } else {
        cmd[CMD_RESLEN_INDEX] |= 0x80; // Device not present
    }
    pif_channel++;
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


INLINE void pif_mempack_read(byte* cmd, byte* res) {
    init_mempack(&n64sys.mem, n64sys.rom_path);
    // First two bytes in the command are the offset
    half offset = CMD_DATA[0] << 8;
    offset |= CMD_DATA[1];

    // low 5 bits are the CRC
    //byte crc = offset & 0x1F;
    // offset must be 32-byte aligned
    offset &= ~0x1F;

    // The most significant bit in the address is not used for the mempak.
    // The game will identify whether the connected device is a mempak or something else by writing to
    // 0x8000, then reading from 0x0000. If the device returns the same data, it's a mempak. Otherwise, it's
    // something else, depending on the data it returns.
    offset &= 0x7FE0;

    switch (get_controller_accessory_type(pif_channel)) {
        case CONTROLLER_ACCESSORY_NONE:
            for (int i = 0; i < 32; i++) {
                res[i] = 0;
            }
            break;
        case CONTROLLER_ACCESSORY_MEMPACK:
            for (int i = 0; i < 32; i++) {
                res[i] = n64sys.mem.mempack_data[offset + i];
            }
            break;
        case CONTROLLER_ACCESSORY_RUMBLE_PACK:
            for (int i = 0; i < 32; i++) {
                res[i] = 0x80;
            }
            break;
    }

    // CRC byte
    res[32] = data_crc(&res[0]);
}

INLINE void pif_mempack_write(byte* cmd, byte* res) {
    // First two bytes in the command are the offset
    half offset = CMD_DATA[0] << 8;
    offset |= CMD_DATA[1];

    // low 5 bits are the CRC
    //byte crc = offset & 0x1F;
    // offset must be 32-byte aligned
    offset &= ~0x1F;

    // The most significant bit in the address is not used for the mempak.
    // The game will identify whether the connected device is a mempak or something else by writing to
    // 0x8000, then reading from 0x0000. If the device returns the same data, it's a mempak. Otherwise, it's
    // something else, depending on the data it returns.
    offset &= 0x7FE0;

    switch (get_controller_accessory_type(pif_channel)) {
        case CONTROLLER_ACCESSORY_NONE:
            return;
        case CONTROLLER_ACCESSORY_MEMPACK:
            init_mempack(&n64sys.mem, n64sys.rom_path);
            for (int i = 0; i < 32; i++) {
                n64sys.mem.mempack_data[offset + i] = CMD_DATA[i + 2];
            }
            n64sys.mem.mempack_data_dirty = true;
            break;
        case CONTROLLER_ACCESSORY_RUMBLE_PACK: {
            bool all_zeroes = true;
            bool all_ones = true;
            for (int i = 0; i < 32; i++) {
                all_zeroes = all_zeroes && CMD_DATA[i + 2] == 0;
                all_ones   = all_ones   && CMD_DATA[i + 2] == 1;
            }
            if (all_zeroes) {
                gamepad_rumble_off(pif_channel);
            } else if (all_ones) {
                gamepad_rumble_on(pif_channel);
            }
            break;
        }
    }
    // CRC byte
    res[0] = data_crc(&CMD_DATA[2]);
}

INLINE void pif_eeprom_read(byte* cmd, byte* res) {
    assert_is_eeprom(n64sys.mem.save_type);
    if (pif_channel == 4) {
        byte offset = CMD_DATA[0];
        if ((offset * 8) >= n64sys.mem.save_size) {
            logfatal("Out of range EEPROM read! offset: 0x%02X", offset);
        }

        for (int i = 0; i < 8; i++) {
            res[i] = n64sys.mem.save_data[(offset * 8) + i];
        }
    } else {
        logfatal("EEPROM read on bad channel %d", pif_channel);
    }
}

INLINE void pif_eeprom_write(byte* cmd, byte* res) {
    assert_is_eeprom(n64sys.mem.save_type);
    if (pif_channel == 4) {
        byte offset = CMD_DATA[0];
        if ((offset * 8) >= n64sys.mem.save_size) {
            logfatal("Out of range EEPROM write! offset: 0x%02X", offset);
        }

        for (int i = 0; i < 8; i++) {
            n64sys.mem.save_data[(offset * 8) + i] = CMD_DATA[1 + i];
        }

        res[0] = 0; // Error byte, I guess it always succeeds?
    } else {
        logfatal("EEPROM write on bad channel %d", pif_channel);
    }
    n64sys.mem.save_data_dirty = true;
}

void process_pif_command() {
    byte control = n64sys.mem.pif_ram[63];
    if (control == 1) {
        pif_channel = 0;
        int i = 0;
        while (i < 63) {
            byte* cmd = &n64sys.mem.pif_ram[i++];
            byte cmdlen = cmd[CMD_CMDLEN_INDEX] & 0x3F;

            if (cmdlen == 0) {
                pif_channel++;
            } else if (cmdlen == 0x3F) {
                continue;
            } else if (cmdlen == 0x3E) {
                break;
            } else {
                byte r = n64sys.mem.pif_ram[i++];
                if (r == 0xFE) {
                    continue;
                }
                byte reslen = r & 0x3F; // TODO: out of bounds access possible on invalid data
                byte* res = &n64sys.mem.pif_ram[i + cmdlen];

                switch (cmd[CMD_COMMAND_INDEX]) {
                    case PIF_COMMAND_RESET:
                        unimplemented(cmdlen != 1, "Reset with cmdlen != 1");
                        unimplemented(reslen != 3, "Reset with reslen != 3");
                        pif_controller_reset(cmd, res);
                        pif_controller_id(cmd, res);
                        break;
                    case PIF_COMMAND_CONTROLLER_ID:
                        unimplemented(cmdlen != 1, "Controller id with cmdlen != 1");
                        unimplemented(reslen != 3, "Controller id with reslen != 3");
                        pif_controller_id(cmd, res);
                        break;
                    case PIF_COMMAND_READ_BUTTONS:
                        unimplemented(cmdlen != 1, "Read buttons with cmdlen != 1");
                        unimplemented(reslen != 4, "Read buttons with reslen != 4");
                        pif_read_buttons(cmd, res);
                        break;
                    case PIF_COMMAND_MEMPACK_READ:
                        unimplemented(cmdlen != 3, "Mempack read with cmdlen != 3");
                        unimplemented(reslen != 33, "Mempack read with reslen != 33");
                        pif_mempack_read(cmd, res);
                        break;
                    case PIF_COMMAND_MEMPACK_WRITE:
                        unimplemented(cmdlen != 35, "Mempack write with cmdlen != 35");
                        unimplemented(reslen != 1, "Mempack write with reslen != 1");
                        pif_mempack_write(cmd, res);
                        break;
                    case PIF_COMMAND_EEPROM_READ:
                        unimplemented(cmdlen != 2, "EEPROM read with cmdlen != 2");
                        unimplemented(reslen != 8, "EEPROM read with reslen != 8");
                        unimplemented(n64sys.mem.save_data == NULL, "EEPROM read when save data is uninitialized! Is this game in the game DB?");
                        pif_eeprom_read(cmd, res);
                        break;
                    case PIF_COMMAND_EEPROM_WRITE:
                        unimplemented(cmdlen != 10, "EEPROM write with cmdlen != 10");
                        unimplemented(reslen != 1,  "EEPROM write with reslen != 1");
                        unimplemented(n64sys.mem.save_data == NULL, "EEPROM write when save data is uninitialized! Is this game in the game DB?");
                        pif_eeprom_write(cmd, res);
                        break;
                    default:
                        logfatal("Invalid PIF command: %X", cmd[CMD_COMMAND_INDEX]);
                }

                i += cmdlen + reslen;
            }
        }
    }
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
