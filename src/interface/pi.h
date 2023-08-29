#ifndef N64_PI_H
#define N64_PI_H
#include <util.h>

#define SREGION_PI_UNKNOWN  0x00000000
#define SREGION_PI_64DD_REG 0x05000000
#define SREGION_PI_64DD_ROM 0x06000000
#define SREGION_PI_SRAM     0x08000000
#define SREGION_PI_ROM      0x10000000

#define EREGION_PI_UNKNOWN  0x04FFFFFF
#define EREGION_PI_64DD_REG 0x05FFFFFF
#define EREGION_PI_64DD_ROM 0x07FFFFFF
#define EREGION_PI_SRAM     0x0FFFFFFF
#define EREGION_PI_ROM      0xFFFFFFFF

#define REGION_PI_UNKNOWN  SREGION_PI_UNKNOWN  ... EREGION_PI_UNKNOWN
#define REGION_PI_64DD_REG SREGION_PI_64DD_REG ... EREGION_PI_64DD_REG
#define REGION_PI_64DD_ROM SREGION_PI_64DD_ROM ... EREGION_PI_64DD_ROM
#define REGION_PI_SRAM     SREGION_PI_SRAM     ... EREGION_PI_SRAM
#define REGION_PI_ROM      SREGION_PI_ROM      ... EREGION_PI_ROM

u32 read_word_pireg(u32 address);
void write_word_pireg(u32 address, u32 value);

void write_byte_pibus(u32 address, u32 value);
u8 read_byte_pibus(u32 address);

void write_half_pibus(u32 address, u16 value);
u16 read_half_pibus(u32 address);

void write_word_pibus(u32 address, u32 value);
u32 read_word_pibus(u32 address);

void write_dword_pibus(u32 address, u64 value);
u64 read_dword_pibus(u32 address);

void on_pi_dma_complete();
void on_pi_write_complete();

#endif //N64_PI_H
