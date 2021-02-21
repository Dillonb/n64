#ifndef SOFTRDP_H
#define SOFTRDP_H
#include <stdint.h>
#include <stdbool.h>
typedef struct softrdp_state {
    struct {
        uint16_t xl;
        uint16_t yl;
        uint16_t xh;
        uint16_t yh;
        bool f;
        bool o;
    } scissor;

    uint16_t primitive_z;
    uint16_t primitive_delta_z;
} softrdp_state_t;

void init_softrdp(softrdp_state_t* state);
void enqueue_command_softrdp(softrdp_state_t* state, int command_length, uint32_t* buffer);
#endif