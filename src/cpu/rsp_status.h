#ifndef N64_RSP_STATUS_H
#define N64_RSP_STATUS_H

#include <stdbool.h>
#include "../common/util.h"

typedef union rsp_status {
    word raw;
    struct {
        bool halt:1;
        bool broke:1;
        bool dma_busy:1;
        bool dma_full:1;
        bool io_full:1;
        bool single_step:1;
        bool intr_on_break:1;
        bool signal_0:1;
        bool signal_1:1;
        bool signal_2:1;
        bool signal_3:1;
        bool signal_4:1;
        bool signal_5:1;
        bool signal_6:1;
        bool signal_7:1;
        unsigned:17;
    };
} rsp_status_t;

#endif //N64_RSP_STATUS_H
