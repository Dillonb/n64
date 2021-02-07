#ifndef N64_METRICS_H
#define N64_METRICS_H
#include <stdint.h>
#include "util.h"

typedef enum metric {
    METRIC_BLOCK_COMPILATION = 0,
    METRIC_RSP_STEPS,
    NUM_METRICS
} metric_t;

extern uint64_t n64_metric_data[NUM_METRICS];

INLINE void mark_metric(metric_t metric) {
    n64_metric_data[metric]++;
}

INLINE void mark_metric_multiple(metric_t metric, int times) {
    n64_metric_data[metric] += times;
}

INLINE uint64_t get_metric(metric_t metric) {
    return n64_metric_data[metric];
}

INLINE void reset_all_metrics() {
    for (int i = 0; i < NUM_METRICS; i++) {
        n64_metric_data[i] = 0;
    }
}

#endif //N64_METRICS_H
