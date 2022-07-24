#ifndef N64_N64ROM_H
#define N64_N64ROM_H
#include <stdio.h>
#include <util.h>
#include <stdbool.h>

typedef struct n64_header {
    u8 initial_values[4];
    u32 clock_rate;
    u32 program_counter;
    u32 release;
    u32 crc1;
    u32 crc2;
    u64 unknown;
    char image_name[20];
    u32 unknown2;
    u32 manufacturer_id;
    u16 cartridge_id;
    char country_code[2];
    u8 boot_code[4032];
} n64_header_t;

typedef enum n64_cic_type {
    UNKNOWN_CIC_TYPE,
    CIC_NUS_6101,
    CIC_NUS_7102,
    CIC_NUS_6102_7101,
    CIC_NUS_6103_7103,
    CIC_NUS_6105_7105,
    CIC_NUS_6106_7106
} n64_cic_type_t;

typedef struct n64_rom {
    u8* rom;
    size_t size;
    u8* pif_rom;
    size_t pif_rom_size;
    n64_header_t header;
    n64_cic_type_t cic_type;
    char game_name_cartridge[20];
    const char* game_name_db;
    char code[4];
    bool pal;
} n64_rom_t;

void load_n64rom(n64_rom_t* rom, const char* path);

#endif //N64_N64ROM_H
