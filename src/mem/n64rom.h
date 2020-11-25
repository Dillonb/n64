#ifndef N64_N64ROM_H
#define N64_N64ROM_H
#include <stdio.h>
#include <util.h>

typedef struct n64_header {
    byte initial_values[4];
    word clock_rate;
    word program_counter;
    word release;
    word crc1;
    word crc2;
    dword unknown;
    char image_name[20];
    word unknown2;
    word manufacturer_id;
    half cartridge_id;
    char country_code[2];
    byte boot_code[4032];
} n64_header_t;

typedef enum n64_cic_type {
    UNKNOWN_CIC_TYPE,
    CIC_NUS_6101_7102,
    CIC_NUS_6102_7101,
    CIC_NUS_6103_7103,
    CIC_NUS_6105_7105,
    CIC_NUS_6106_7106
} n64_cic_type_t;

typedef struct n64_rom {
    byte* rom;
    size_t size;
    byte* pif_rom;
    size_t pif_rom_size;
    n64_header_t header;
    n64_cic_type_t cic_type;
} n64_rom_t;

void load_n64rom(n64_rom_t* rom, const char* path);

#endif //N64_N64ROM_H
