#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <log.h>
#include <zlib.h>
#include <frontend/game_db.h>
#include "n64rom.h"
#include "mem_util.h"

#define Z64_IDENTIFIER 0x80371240
#define N64_IDENTIFIER 0x40123780
#define V64_IDENTIFIER 0x37804012

void byteswap_to_be(byte* rom, size_t rom_size) {
    word identifier;
    memcpy(&identifier, rom, 4); // first 4 bytes
    identifier = be32toh(identifier);

    switch(identifier) {
        case Z64_IDENTIFIER:
            loginfo("This is a .z64 ROM, no byte swapping is needed.");
            return;
        case N64_IDENTIFIER:
            for (int i = 0; i < rom_size / 4; i++) {
                word w;
                memcpy(&w, rom + (i * 4), 4);
                word swapped = __bswap_32(w);
                memcpy(rom + (i * 4), &swapped, 4);
            }
            loginfo("This is a .n64 file, byte swapping it!");
            break;
        case V64_IDENTIFIER:
            for (int i = 0; i < rom_size / 4; i++) {
                word w;
                memcpy(&w, rom + (i * 4), 4);
                word swapped = (0xFF000000 & (w << 8)) |
                        (0x00FF0000 & (w >> 8)) |
                        (0x0000FF00 & (w << 8)) |
                        (0x000000FF & (w >> 8));
                memcpy(rom + (i * 4), &swapped, 4);
            }
            loginfo("This is a .v64 file, byte swapping it!");
            return;
        default:
            logfatal("Invalid cartridge header! This does not look like a valid N64 ROM.\n");
    }
}

void byteswap_to_host(byte* rom, size_t rom_size) {
#ifndef N64_BIG_ENDIAN // Only need to swap if not already on a big endian host
    for (int i = 0; i < rom_size / 4; i++) {
        word w;
        memcpy(&w, rom + (i * 4), 4);
        word swapped = be32toh(w);
        memcpy(rom + (i * 4), &swapped, 4);
    }
#endif
}

void load_n64rom(n64_rom_t* rom, const char* path) {
    if (rom->rom != NULL) {
        free(rom->rom);
        rom->rom = NULL;
    }
    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        logfatal("Error opening the file! Are you sure it's a valid ROM and that it exists?");
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if (size < sizeof(n64_header_t)) {
        logfatal("This file looks way too small to be a valid N64 ROM!");
    }
    fseek(fp, 0, SEEK_SET);
    byte *buf = malloc(size);
    fread(buf, size, 1, fp);

    byteswap_to_be(buf, size);

    rom->rom = buf;
    rom->size = size;
    memcpy(&rom->header, buf, sizeof(n64_header_t));
    memcpy(rom->game_name_cartridge, rom->header.image_name, sizeof(rom->header.image_name));

    rom->header.clock_rate = be32toh(rom->header.clock_rate);
    rom->header.program_counter = be32toh(rom->header.program_counter);
    rom->header.release = be32toh(rom->header.release);
    rom->header.crc1 = be32toh(rom->header.crc1);
    rom->header.crc2 = be32toh(rom->header.crc2);
    rom->header.unknown = be64toh(rom->header.unknown);
    rom->header.unknown2 = be32toh(rom->header.unknown2);
    rom->header.manufacturer_id = be32toh(rom->header.manufacturer_id);
    rom->header.cartridge_id = be16toh(rom->header.cartridge_id);


    rom->code[0] = rom->header.manufacturer_id & 0xFF;
    rom->code[1] = (rom->header.cartridge_id >> 8) & 0xFF;
    rom->code[2] = rom->header.cartridge_id & 0xFF;
    rom->code[3] = '\0';

    // Strip trailing spaces
    for (int i = sizeof(rom->header.image_name) - 1; rom->game_name_cartridge[i] == ' '; i--) {
        rom->game_name_cartridge[i] = '\0';
    }

    word checksum = crc32(0, &rom->rom[0x40], 0x9c0);

    switch (checksum) {
        case 0x1DEB51A9:
            rom->cic_type = CIC_NUS_6101_7102;
            break;

        case 0xC08E5BD6:
            rom->cic_type = CIC_NUS_6102_7101;
            break;

        case 0x03B8376A:
            rom->cic_type = CIC_NUS_6103_7103;
            break;

        case 0xCF7F41DC:
            rom->cic_type = CIC_NUS_6105_7105;
            break;

        case 0xD1059C6A:
            rom->cic_type = CIC_NUS_6106_7106;
            break;

        default:
            logwarn("Could not determine CIC TYPE! Checksum: 0x%08X is unknown!\n", checksum);
            rom->cic_type = UNKNOWN_CIC_TYPE;
            break;
    }


    byteswap_to_host(rom->rom, size);

    loginfo("Loaded %s", rom->game_name_cartridge);
    logdebug("The program counter starts at: " PRINTF_WORD, rom->header.program_counter);
}
