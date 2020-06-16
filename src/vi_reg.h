#ifndef N64_VI_REG_H
#define N64_VI_REG_H

#include <stdbool.h>
#include "common/util.h"

typedef union vi_status {
    word raw;
    struct {
        byte type:2;
        bool gamma_dither_enable:1;
        bool gamma_enable:1;
        bool divot_enable:1;
        bool reserved_always_off:1;
        bool serrate:1;
        bool reserved_diagnostics_only:1;
        unsigned antialias_mode:3;
        unsigned:21;
    };
} vi_status_t;

typedef union vi_burst {
    word raw;
} vi_burst_t;

#endif //N64_VI_REG_H
