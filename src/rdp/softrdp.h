#ifndef SOFTRDP_H
#define SOFTRDP_H
#include <stdint.h>
void init_softrdp();
void enqueue_command_softrdp(int command_length, uint32_t* buffer);
#endif