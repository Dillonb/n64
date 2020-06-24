#ifndef N64_N64SYSTEM_H
#define N64_N64SYSTEM_H
#include <stdbool.h>
#include "../mem/n64mem.h"
#include "../cpu/r4300i.h"
#include "../cpu/rsp_status.h"
#include "../vi_reg.h"

typedef enum n64_interrupt {
    INTERRUPT_VI,
    INTERRUPT_SI,
    INTERRUPT_PI,
    INTERRUPT_DP,
} n64_interrupt_t;

typedef union mi_intr_mask {
    word raw;
    struct {
        bool sp:1;
        bool si:1;
        bool ai:1;
        bool vi:1;
        bool pi:1;
        bool dp:1;
        unsigned:26;
    };
} mi_intr_mask_t;

typedef union mi_intr {
    word raw;
    struct {
        bool sp:1;
        bool si:1;
        bool ai:1;
        bool vi:1;
        bool pi:1;
        bool dp:1;
        unsigned:26;
    };
} mi_intr_t;

typedef struct n64_system {
    n64_mem_t mem;
    r4300i_t cpu;
    r4300i_t rsp;
    rsp_status_t rsp_status;
    struct {
        word init_mode;
        mi_intr_mask_t intr_mask;
        mi_intr_t intr;
    } mi;
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
void n64_system_loop(n64_system_t* system);
void n64_system_cleanup(n64_system_t* system);
void n64_request_quit();
void interrupt_raise(n64_system_t* system, n64_interrupt_t interrupt);
void interrupt_lower(n64_system_t* system, n64_interrupt_t interrupt);
#endif //N64_N64SYSTEM_H
