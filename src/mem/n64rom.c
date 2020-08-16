#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <log.h>
#include "n64rom.h"

void load_n64rom(n64_rom_t* rom, const char* path) {
    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        logfatal("Error opening the file! Are you sure it's a valid ROM and that it exists?")
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if (size < sizeof(n64_header_t)) {
        logfatal("This file looks way too small to be a valid N64 ROM!")
    }
    fseek(fp, 0, SEEK_SET);
    byte *buf = malloc(size);
    fread(buf, size, 1, fp);

    rom->rom = buf;
    rom->size = size;
    memcpy(&rom->header, buf, sizeof(n64_header_t));

    rom->header.program_counter = be32toh(rom->header.program_counter);

    loginfo("Loaded %s", rom->header.image_name)
    logdebug("The program counter starts at: " PRINTF_WORD, rom->header.program_counter);
}
