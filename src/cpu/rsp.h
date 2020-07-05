#ifndef N64_RSP_H
#define N64_RSP_H

#include "../common/util.h"
#include "../system/n64system.h"

word read_word_spreg(n64_system_t* system, word address);
void write_word_spreg(n64_system_t* system, word address, word value);
void rsp_step(r4300i_t* rsp);

#endif //N64_RSP_H
