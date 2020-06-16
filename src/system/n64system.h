#ifndef N64_N64SYSTEM_H
#define N64_N64SYSTEM_H
#include <stdbool.h>
#include "../mem/n64mem.h"
#include "../cpu/r4300i.h"
#include "../cpu/rsp_status.h"
#include "../vi_reg.h"

typedef struct n64_system {
    n64_mem_t mem;
    r4300i_t cpu;
    r4300i_t rsp;
    rsp_status_t rsp_status;
    struct {
        vi_status_t status;
        word vi_origin;
        word vi_width;
        word vi_v_intr;
        vi_burst_t vi_burst;
        word vsync;
        word hsync;
        word leap;
        word hstart;
        word vstart;
        word vburst;
        word xscale;
        word yscale;
        word v_current;
    } vi;
} n64_system_t;

n64_system_t* init_n64system(const char* rom_path, bool enable_frontend);

void n64_system_step(n64_system_t* system);
_Noreturn void n64_system_loop(n64_system_t* system);
void n64_system_cleanup(n64_system_t* system);
#endif //N64_N64SYSTEM_H
