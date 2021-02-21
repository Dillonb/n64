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
    uint32_t fill_color;
    struct {
        uint8_t format;
        uint8_t size;
        uint16_t width;
        uint32_t dram_addr;
    } color_image;
} softrdp_state_t;

void init_softrdp(softrdp_state_t* state);
void enqueue_command_softrdp(softrdp_state_t* state, int command_length, uint32_t* buffer);
#endif