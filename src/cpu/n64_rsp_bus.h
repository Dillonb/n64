#ifndef N64_N64_RSP_BUS_H
#define N64_N64_RSP_BUS_H

#include <util.h>
#include <cpu/rsp_types.h>

word n64_rsp_read_word(word address);
void n64_rsp_write_word(word address, word value);

half n64_rsp_read_half(word address);
void n64_rsp_write_half(word address, half value);

byte n64_rsp_read_byte(word address);
void n64_rsp_write_byte(word address, byte value);

#endif //N64_N64_RSP_BUS_H
