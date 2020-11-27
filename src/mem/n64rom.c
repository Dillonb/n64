#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <log.h>
#include <zlib.h>
#include "n64rom.h"
#include "mem_util.h"

void load_n64rom(n64_rom_t* rom, const char* path) {
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

    rom->rom = buf;
    rom->size = size;
    memcpy(&rom->header, buf, sizeof(n64_header_t));
    memcpy(rom->game_name, rom->header.image_name, sizeof(rom->header.image_name));

    // Strip trailing spaces
    for (int i = sizeof(rom->header.image_name) - 1; rom->game_name[i] == ' '; i--) {
        rom->game_name[i] = '\0';
    }

    printf("'%s'\n", rom->game_name);

    rom->header.program_counter = be32toh(rom->header.program_counter);

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



    loginfo("Loaded %s", rom->game_name);
    logdebug("The program counter starts at: " PRINTF_WORD, rom->header.program_counter);
}
