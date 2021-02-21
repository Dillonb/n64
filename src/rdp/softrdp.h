#ifndef SOFTRDP_H
#define SOFTRDP_H
#include <stdint.h>
typedef struct softrdp_state {

} softrdp_state_t;

void init_softrdp(softrdp_state_t* state);
void enqueue_command_softrdp(softrdp_state_t* state, int command_length, uint32_t* buffer);
#endif