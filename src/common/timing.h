#ifndef N64_TIMING_H
#define N64_TIMING_H

#include "util.h"

extern unsigned int extra_cycles;

#define SI_DMA_DELAY (65536 * 2)
#define PI_BUS_WRITE 100
u32 timing_pi_access(u8 domain, u32 length);

INLINE void cpu_stall(unsigned int cycles) {
    extra_cycles += cycles;
}

INLINE unsigned int pop_stalled_cycles() {
    unsigned int temp = extra_cycles;
    extra_cycles = 0;
    return temp;
}

#endif //N64_TIMING_H
