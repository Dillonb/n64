#include <frontend/tas_movie.h>
#include <frontend/gamepad.h>
#include <rsp.h>
#include <n64_cic_nus_6105.h>
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

#define MEMPACK_SIZE 0x8000

#define CMD_CMDLEN_INDEX 0
#define CMD_RESLEN_INDEX 1
#define CMD_COMMAND_INDEX 2
#define CMD_START_INDEX 3

#define CMD_DATA (&cmd[CMD_START_INDEX])

const u32 cic_seeds[] = {
        0x0,
        0x00043F3F, // CIC_NUS_6101
        0x00043F3F, // CIC_NUS_7102
        0x00003F3F, // CIC_NUS_6102_7101
        0x0000783F, // CIC_NUS_6103_7103
        0x0000913F, // CIC_NUS_6105_7105
        0x0000853F, // CIC_NUS_6106_7106
};

static int pif_channel = 0;

void pif_rom_execute_hle() {
    switch (n64sys.mem.rom.cic_type) {
        case UNKNOWN_CIC_TYPE:
            logalways("Unknown CIC type, the game may not boot! Assuming 6102 and hoping for the best...");

        case CIC_NUS_6101:
            N64CPU.gpr[0] = 0x0000000000000000;
            N64CPU.gpr[1] = 0x0000000000000000;
            N64CPU.gpr[2] = 0xFFFFFFFFDF6445CC;
            N64CPU.gpr[3] = 0xFFFFFFFFDF6445CC;
            N64CPU.gpr[4] = 0x00000000000045CC;
            N64CPU.gpr[5] = 0x0000000073EE317A;
            N64CPU.gpr[6] = 0xFFFFFFFFA4001F0C;
            N64CPU.gpr[7] = 0xFFFFFFFFA4001F08;
            N64CPU.gpr[8] = 0x00000000000000C0;
            N64CPU.gpr[9] = 0x0000000000000000;
            N64CPU.gpr[10] = 0x0000000000000040;
            N64CPU.gpr[11] = 0xFFFFFFFFA4000040;
            N64CPU.gpr[12] = 0xFFFFFFFFC7601FAC;
            N64CPU.gpr[13] = 0xFFFFFFFFC7601FAC;
            N64CPU.gpr[14] = 0xFFFFFFFFB48E2ED6;
            N64CPU.gpr[15] = 0xFFFFFFFFBA1A7D4B;
            N64CPU.gpr[16] = 0x0000000000000000;
            N64CPU.gpr[17] = 0x0000000000000000;
            N64CPU.gpr[18] = 0x0000000000000000;
            N64CPU.gpr[19] = 0x0000000000000000;
            N64CPU.gpr[20] = 0x0000000000000001;
            N64CPU.gpr[21] = 0x0000000000000000;
            N64CPU.gpr[23] = 0x0000000000000001;
            N64CPU.gpr[24] = 0x0000000000000002;
            N64CPU.gpr[25] = 0xFFFFFFFF905F4718;
            N64CPU.gpr[26] = 0x0000000000000000;
            N64CPU.gpr[27] = 0x0000000000000000;
            N64CPU.gpr[28] = 0x0000000000000000;
            N64CPU.gpr[29] = 0xFFFFFFFFA4001FF0;
            N64CPU.gpr[30] = 0x0000000000000000;
            N64CPU.gpr[31] = 0xFFFFFFFFA4001550;

            N64CPU.mult_lo = 0xFFFFFFFFBA1A7D4B;
            N64CPU.mult_hi = 0xFFFFFFFF997EC317;
            break;

        case CIC_NUS_7102:
            N64CPU.gpr[0] = 0x0000000000000000;
            N64CPU.gpr[1] = 0x0000000000000001;
            N64CPU.gpr[2] = 0x000000001E324416;
            N64CPU.gpr[3] = 0x000000001E324416;
            N64CPU.gpr[4] = 0x0000000000004416;
            N64CPU.gpr[5] = 0x000000000EC5D9AF;
            N64CPU.gpr[6] = 0xFFFFFFFFA4001F0C;
            N64CPU.gpr[7] = 0xFFFFFFFFA4001F08;
            N64CPU.gpr[8] = 0x00000000000000C0;
            N64CPU.gpr[9] = 0x0000000000000000;
            N64CPU.gpr[10] = 0x0000000000000040;
            N64CPU.gpr[11] = 0xFFFFFFFFA4000040;
            N64CPU.gpr[12] = 0x00000000495D3D7B;
            N64CPU.gpr[13] = 0xFFFFFFFF8B3DFA1E;
            N64CPU.gpr[14] = 0x000000004798E4D4;
            N64CPU.gpr[15] = 0xFFFFFFFFF1D30682;
            N64CPU.gpr[16] = 0x0000000000000000;
            N64CPU.gpr[17] = 0x0000000000000000;
            N64CPU.gpr[18] = 0x0000000000000000;
            N64CPU.gpr[19] = 0x0000000000000000;
            N64CPU.gpr[20] = 0x0000000000000000;
            N64CPU.gpr[21] = 0x0000000000000000;
            N64CPU.gpr[22] = 0x000000000000003F;
            N64CPU.gpr[23] = 0x0000000000000007;
            N64CPU.gpr[24] = 0x0000000000000000;
            N64CPU.gpr[25] = 0x0000000013D05CAB;
            N64CPU.gpr[26] = 0x0000000000000000;
            N64CPU.gpr[27] = 0x0000000000000000;
            N64CPU.gpr[28] = 0x0000000000000000;
            N64CPU.gpr[29] = 0xFFFFFFFFA4001FF0;
            N64CPU.gpr[30] = 0x0000000000000000;
            N64CPU.gpr[31] = 0xFFFFFFFFA4001554;

            N64CPU.mult_lo = 0xFFFFFFFFF1D30682;
            N64CPU.mult_hi = 0x0000000010054A98;
            break;

        case CIC_NUS_6102_7101:
            N64CPU.gpr[0] = 0x0000000000000000;
            N64CPU.gpr[1] = 0x0000000000000001;
            N64CPU.gpr[2] = 0x000000000EBDA536;
            N64CPU.gpr[3] = 0x000000000EBDA536;
            N64CPU.gpr[4] = 0x000000000000A536;
            N64CPU.gpr[5] = 0xFFFFFFFFC0F1D859;
            N64CPU.gpr[6] = 0xFFFFFFFFA4001F0C;
            N64CPU.gpr[7] = 0xFFFFFFFFA4001F08;
            N64CPU.gpr[8] = 0x00000000000000C0;
            N64CPU.gpr[9] = 0x0000000000000000;
            N64CPU.gpr[10] = 0x0000000000000040;
            N64CPU.gpr[11] = 0xFFFFFFFFA4000040;
            N64CPU.gpr[12] = 0xFFFFFFFFED10D0B3;
            N64CPU.gpr[13] = 0x000000001402A4CC;
            N64CPU.gpr[14] = 0x000000002DE108EA;
            N64CPU.gpr[15] = 0x000000003103E121;
            N64CPU.gpr[16] = 0x0000000000000000;
            N64CPU.gpr[17] = 0x0000000000000000;
            N64CPU.gpr[18] = 0x0000000000000000;
            N64CPU.gpr[19] = 0x0000000000000000;
            N64CPU.gpr[20] = 0x0000000000000001;
            N64CPU.gpr[21] = 0x0000000000000000;
            N64CPU.gpr[23] = 0x0000000000000000;
            N64CPU.gpr[24] = 0x0000000000000000;
            N64CPU.gpr[25] = 0xFFFFFFFF9DEBB54F;
            N64CPU.gpr[26] = 0x0000000000000000;
            N64CPU.gpr[27] = 0x0000000000000000;
            N64CPU.gpr[28] = 0x0000000000000000;
            N64CPU.gpr[29] = 0xFFFFFFFFA4001FF0;
            N64CPU.gpr[30] = 0x0000000000000000;
            N64CPU.gpr[31] = 0xFFFFFFFFA4001550;

            N64CPU.mult_hi = 0x000000003FC18657;
            N64CPU.mult_lo = 0x000000003103E121;

            if (n64sys.mem.rom.pal) {
                N64CPU.gpr[20] = 0x0000000000000000;
                N64CPU.gpr[23] = 0x0000000000000006;
                N64CPU.gpr[31] = 0xFFFFFFFFA4001554;
            }

            break;

        case CIC_NUS_6103_7103:
            N64CPU.gpr[0] = 0x0000000000000000;
            N64CPU.gpr[1] = 0x0000000000000001;
            N64CPU.gpr[2] = 0x0000000049A5EE96;
            N64CPU.gpr[3] = 0x0000000049A5EE96;
            N64CPU.gpr[4] = 0x000000000000EE96;
            N64CPU.gpr[5] = 0xFFFFFFFFD4646273;
            N64CPU.gpr[6] = 0xFFFFFFFFA4001F0C;
            N64CPU.gpr[7] = 0xFFFFFFFFA4001F08;
            N64CPU.gpr[8] = 0x00000000000000C0;
            N64CPU.gpr[9] = 0x0000000000000000;
            N64CPU.gpr[10] = 0x0000000000000040;
            N64CPU.gpr[11] = 0xFFFFFFFFA4000040;
            N64CPU.gpr[12] = 0xFFFFFFFFCE9DFBF7;
            N64CPU.gpr[13] = 0xFFFFFFFFCE9DFBF7;
            N64CPU.gpr[14] = 0x000000001AF99984;
            N64CPU.gpr[15] = 0x0000000018B63D28;
            N64CPU.gpr[16] = 0x0000000000000000;
            N64CPU.gpr[17] = 0x0000000000000000;
            N64CPU.gpr[18] = 0x0000000000000000;
            N64CPU.gpr[19] = 0x0000000000000000;
            N64CPU.gpr[20] = 0x0000000000000001;
            N64CPU.gpr[21] = 0x0000000000000000;
            N64CPU.gpr[23] = 0x0000000000000000;
            N64CPU.gpr[24] = 0x0000000000000000;
            N64CPU.gpr[25] = 0xFFFFFFFF825B21C9;
            N64CPU.gpr[26] = 0x0000000000000000;
            N64CPU.gpr[27] = 0x0000000000000000;
            N64CPU.gpr[28] = 0x0000000000000000;
            N64CPU.gpr[29] = 0xFFFFFFFFA4001FF0;
            N64CPU.gpr[30] = 0x0000000000000000;
            N64CPU.gpr[31] = 0xFFFFFFFFA4001550;

            N64CPU.mult_lo = 0x0000000018B63D28;
            N64CPU.mult_hi = 0x00000000625C2BBE;

            if (n64sys.mem.rom.pal) {
                N64CPU.gpr[20] = 0x0000000000000000;
                N64CPU.gpr[23] = 0x0000000000000006;
                N64CPU.gpr[31] = 0xFFFFFFFFA4001554;
            }

            break;

        case CIC_NUS_6105_7105:
            N64CPU.gpr[0] = 0x0000000000000000;
            N64CPU.gpr[1] = 0x0000000000000000;
            N64CPU.gpr[2] = 0xFFFFFFFFF58B0FBF;
            N64CPU.gpr[3] = 0xFFFFFFFFF58B0FBF;
            N64CPU.gpr[4] = 0x0000000000000FBF;
            N64CPU.gpr[5] = 0xFFFFFFFFDECAAAD1;
            N64CPU.gpr[6] = 0xFFFFFFFFA4001F0C;
            N64CPU.gpr[7] = 0xFFFFFFFFA4001F08;
            N64CPU.gpr[8] = 0x00000000000000C0;
            N64CPU.gpr[9] = 0x0000000000000000;
            N64CPU.gpr[10] = 0x0000000000000040;
            N64CPU.gpr[11] = 0xFFFFFFFFA4000040;
            N64CPU.gpr[12] = 0xFFFFFFFF9651F81E;
            N64CPU.gpr[13] = 0x000000002D42AAC5;
            N64CPU.gpr[14] = 0x00000000489B52CF;
            N64CPU.gpr[15] = 0x0000000056584D60;
            N64CPU.gpr[16] = 0x0000000000000000;
            N64CPU.gpr[17] = 0x0000000000000000;
            N64CPU.gpr[18] = 0x0000000000000000;
            N64CPU.gpr[19] = 0x0000000000000000;
            N64CPU.gpr[20] = 0x0000000000000001;
            N64CPU.gpr[21] = 0x0000000000000000;
            N64CPU.gpr[23] = 0x0000000000000000;
            N64CPU.gpr[24] = 0x0000000000000002;
            N64CPU.gpr[25] = 0xFFFFFFFFCDCE565F;
            N64CPU.gpr[26] = 0x0000000000000000;
            N64CPU.gpr[27] = 0x0000000000000000;
            N64CPU.gpr[28] = 0x0000000000000000;
            N64CPU.gpr[29] = 0xFFFFFFFFA4001FF0;
            N64CPU.gpr[30] = 0x0000000000000000;
            N64CPU.gpr[31] = 0xFFFFFFFFA4001550;

            N64CPU.mult_lo = 0x0000000056584D60;
            N64CPU.mult_hi = 0x000000004BE35D1F;

            if (n64sys.mem.rom.pal) {
                N64CPU.gpr[20] = 0x0000000000000000;
                N64CPU.gpr[23] = 0x0000000000000006;
                N64CPU.gpr[31] = 0xFFFFFFFFA4001554;
            }

            n64_write_physical_word(SREGION_SP_IMEM + 0x0000, 0x3C0DBFC0);
            n64_write_physical_word(SREGION_SP_IMEM + 0x0004, 0x8DA807FC);
            n64_write_physical_word(SREGION_SP_IMEM + 0x0008, 0x25AD07C0);
            n64_write_physical_word(SREGION_SP_IMEM + 0x000C, 0x31080080);
            n64_write_physical_word(SREGION_SP_IMEM + 0x0010, 0x5500FFFC);
            n64_write_physical_word(SREGION_SP_IMEM + 0x0014, 0x3C0DBFC0);
            n64_write_physical_word(SREGION_SP_IMEM + 0x0018, 0x8DA80024);
            n64_write_physical_word(SREGION_SP_IMEM + 0x001C, 0x3C0BB000);
            break;

        case CIC_NUS_6106_7106:
            N64CPU.gpr[0] = 0x0000000000000000;
            N64CPU.gpr[1] = 0x0000000000000000;
            N64CPU.gpr[2] = 0xFFFFFFFFA95930A4;
            N64CPU.gpr[3] = 0xFFFFFFFFA95930A4;
            N64CPU.gpr[4] = 0x00000000000030A4;
            N64CPU.gpr[5] = 0xFFFFFFFFB04DC903;
            N64CPU.gpr[6] = 0xFFFFFFFFA4001F0C;
            N64CPU.gpr[7] = 0xFFFFFFFFA4001F08;
            N64CPU.gpr[8] = 0x00000000000000C0;
            N64CPU.gpr[9] = 0x0000000000000000;
            N64CPU.gpr[10] = 0x0000000000000040;
            N64CPU.gpr[11] = 0xFFFFFFFFA4000040;
            N64CPU.gpr[12] = 0xFFFFFFFFBCB59510;
            N64CPU.gpr[13] = 0xFFFFFFFFBCB59510;
            N64CPU.gpr[14] = 0x000000000CF85C13;
            N64CPU.gpr[15] = 0x000000007A3C07F4;
            N64CPU.gpr[16] = 0x0000000000000000;
            N64CPU.gpr[17] = 0x0000000000000000;
            N64CPU.gpr[18] = 0x0000000000000000;
            N64CPU.gpr[19] = 0x0000000000000000;
            N64CPU.gpr[20] = 0x0000000000000001;
            N64CPU.gpr[21] = 0x0000000000000000;
            N64CPU.gpr[23] = 0x0000000000000000;
            N64CPU.gpr[24] = 0x0000000000000002;
            N64CPU.gpr[25] = 0x00000000465E3F72;
            N64CPU.gpr[26] = 0x0000000000000000;
            N64CPU.gpr[27] = 0x0000000000000000;
            N64CPU.gpr[28] = 0x0000000000000000;
            N64CPU.gpr[29] = 0xFFFFFFFFA4001FF0;
            N64CPU.gpr[30] = 0x0000000000000000;
            N64CPU.gpr[31] = 0xFFFFFFFFA4001550;

            N64CPU.mult_lo = 0x000000007A3C07F4;
            N64CPU.mult_hi = 0x0000000023953898;

            if (n64sys.mem.rom.pal) {
                N64CPU.gpr[20] = 0x0000000000000000;
                N64CPU.gpr[23] = 0x0000000000000006;
                N64CPU.gpr[31] = 0xFFFFFFFFA4001554;
            }

            break;
    }

    N64CPU.gpr[22] = (cic_seeds[n64sys.mem.rom.cic_type] >> 8) & 0xFF; // bits 8 through 15 of the cic seed goes here for all CIC types.

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
    N64CP0.status.raw    = 0x241000E0;
    //N64CP0.cause.raw     = 0;
    //N64CP0.EPC           = 0;
    N64CP0.PRId          = 0x00000B22;
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
    memcpy(N64RSP.sp_dmem, n64sys.mem.rom.rom, sizeof(u8) * 0x1000);

    set_pc_word_r4300i(0xA4000040);
}

void pif_rom_execute_lle() {
    set_pc_word_r4300i(0x1FC00000 + SVREGION_KSEG1);
}

void pif_rom_execute() {
    n64_write_physical_word(SREGION_PIF_RAM + 0x24, cic_seeds[n64sys.mem.rom.cic_type]);
    switch (n64sys.mem.rom.cic_type) {
        case CIC_NUS_6101:
            logalways("Initializing PIF and CIC: CIC_NUS_6101");
            n64_write_physical_word(SREGION_RDRAM + 0x318, N64_RDRAM_SIZE);
            break;
        case CIC_NUS_7102:
            logalways("Initializing PIF and CIC: CIC_NUS_7102");
            n64_write_physical_word(SREGION_RDRAM + 0x318, N64_RDRAM_SIZE);
            break;
        case CIC_NUS_6102_7101:
            logalways("Initializing PIF and CIC: CIC_NUS_6102_7101");
            n64_write_physical_word(SREGION_RDRAM + 0x318, N64_RDRAM_SIZE);
            break;
        case CIC_NUS_6103_7103:
            logalways("Initializing PIF and CIC: CIC_NUS_6103_7103");
            n64_write_physical_word(SREGION_RDRAM + 0x318, N64_RDRAM_SIZE);
            break;
        case CIC_NUS_6105_7105:
            logalways("Initializing PIF and CIC: CIC_NUS_6105_7105");
            n64_write_physical_word(SREGION_RDRAM + 0x3F0, N64_RDRAM_SIZE);
            break;
        case CIC_NUS_6106_7106:
            logalways("Initializing PIF and CIC: CIC_NUS_6106_7106");
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

INLINE void pif_channel_reset(int channel) {

}

INLINE void pif_controller_reset(u8* cmd, u8* res) {

}

INLINE void pif_controller_id(u8* cmd, u8* res) {
    device_id_for_pif(pif_channel, res);
    pif_channel++;
}

INLINE void pif_read_buttons(u8* cmd, u8* res) {
    if (device_read_buttons_for_pif(pif_channel, res)) {
        cmd[CMD_RESLEN_INDEX] |= 0x00; // Success!
    } else {
        cmd[CMD_RESLEN_INDEX] |= 0x80; // Device not present
    }
    pif_channel++;
}

u8 data_crc(const u8* data) {
    u8 crc = 0;
    for (int i = 0; i <= 32; i++) {
        for (int j = 7; j >= 0; j--) {
            u8 xor_val = ((crc & 0x80) != 0) ? 0x85 : 0x00;

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


INLINE void pif_mempack_read(u8* cmd, u8* res) {
    init_mempack(&n64sys.mem, n64sys.rom_path);
    // First two bytes in the command are the offset
    u16 offset = CMD_DATA[0] << 8;
    offset |= CMD_DATA[1];

    // low 5 bits are the CRC
    //byte crc = offset & 0x1F;
    // offset must be 32-byte aligned
    offset &= ~0x1F;

    switch (get_controller_accessory_type(pif_channel)) {
        case CONTROLLER_ACCESSORY_NONE:
            break;
        case CONTROLLER_ACCESSORY_MEMPACK:
            if (offset <= MEMPACK_SIZE - 0x20) {
                for (int i = 0; i < 32; i++) {
                    res[i] = n64sys.mem.mempack_data[offset + i];
                }
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

INLINE void pif_mempack_write(u8* cmd, u8* res) {
    // First two bytes in the command are the offset
    u16 offset = CMD_DATA[0] << 8;
    offset |= CMD_DATA[1];

    // low 5 bits are the CRC
    //byte crc = offset & 0x1F;
    // offset must be 32-byte aligned
    offset &= ~0x1F;

    switch (get_controller_accessory_type(pif_channel)) {
        case CONTROLLER_ACCESSORY_NONE:
            break;
        case CONTROLLER_ACCESSORY_MEMPACK:
            if (offset <= MEMPACK_SIZE - 0x20) {
                init_mempack(&n64sys.mem, n64sys.rom_path);
                bool data_changed = false;
                for (int i = 0; i < 32; i++) {
                    data_changed |= (n64sys.mem.mempack_data[offset + i] != CMD_DATA[i + 2]);
                    n64sys.mem.mempack_data[offset + i] = CMD_DATA[i + 2];
                }
                n64sys.mem.mempack_data_dirty |= data_changed;
            }
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

INLINE void pif_eeprom_read(u8* cmd, u8* res) {
    assert_is_eeprom(n64sys.mem.save_type);
    if (pif_channel == 4) {
        u8 offset = CMD_DATA[0];
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

INLINE void pif_eeprom_write(u8* cmd, u8* res) {
    assert_is_eeprom(n64sys.mem.save_type);
    if (pif_channel == 4) {
        u8 offset = CMD_DATA[0];
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

void cic_challenge() {
    u8 challenge[30];
    u8 response[30];

    printf("CIC challenge: ");

    // Split 15 bytes into 30 nibbles
    for (int i = 0; i < 15; i++) {
        printf("%02X", n64sys.mem.pif_ram[0x30 + i]);

        challenge[i * 2 + 0]   = (n64sys.mem.pif_ram[0x30 + i] >> 4) & 0x0F;
        challenge[i * 2 + 1]  =  (n64sys.mem.pif_ram[0x30 + i] >> 0) & 0x0F;
    }
    printf("\n");

    n64_cic_nus_6105((char*)challenge, (char*)response, CHL_LEN - 2);

    printf("Challenge response: ");
    for (int i = 0; i < 15; i++) {
        n64sys.mem.pif_ram[0x30 + i] = (response[i * 2] << 4) + response[i * 2 + 1];
        printf("%02X", n64sys.mem.pif_ram[0x30 + i]);
    }
    printf("\n");
}

const char* pif_ram_as_str() {
    static char buf[129];
    memset(buf, 0x00, 129);
    for (int i = 0; i < 64; i++) {
        sprintf(buf + (i * 2), "%02X", n64sys.mem.pif_ram[i]);
    }
    return buf;
}

void process_pif_command() {
    u8 control = n64sys.mem.pif_ram[63];
    if (control & 1) {
        pif_channel = 0;
        int i = 0;
        while (i < 63) {
            u8* cmd = &n64sys.mem.pif_ram[i++];
            u8 cmdlen = cmd[CMD_CMDLEN_INDEX] & 0x3F;

            if (cmdlen == 0) {
                pif_channel++;
            } else if (cmdlen == 0x3D) { // 0xFD in PIF RAM = send reset signal to this pif channel
                pif_channel_reset(pif_channel);
                pif_channel++;
            } else if (cmdlen == 0x3E) { // 0xFE in PIF RAM = end of commands
                break;
            } else if (cmdlen == 0x3F) {
                continue;
            } else {
                u8 r = n64sys.mem.pif_ram[i++];
                if (r == 0xFE) { // 0xFE in PIF RAM = end of commands.
                    break;
                }
                u8 reslen = r & 0x3F; // TODO: out of bounds access possible on invalid data
                u8* res = &n64sys.mem.pif_ram[i + cmdlen];

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

    if (control & 2) {
        cic_challenge();
        n64sys.mem.pif_ram[63] &= ~2;
    }

    if (control & 0x08) {
        n64sys.mem.pif_ram[63] &= ~8;
    }

    if (control & 0x30) {
        n64sys.mem.pif_ram[63] = 0x80;
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
    u8 *buf = malloc(size);
    fread(buf, size, 1, fp);

    n64sys.mem.rom.pif_rom = buf;
    n64sys.mem.rom.pif_rom_size = size;
}
