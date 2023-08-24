#ifndef N64_METRICS_H
#define N64_METRICS_H
#include <stdint.h>
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum metric {
    METRIC_BLOCK_COMPILATION = 0,
    METRIC_RSP_STEPS,
    METRIC_AUDIOSTREAM_AVAILABLE,
    METRIC_SI_INTERRUPT,
    METRIC_PI_INTERRUPT,
    METRIC_AI_INTERRUPT,
    METRIC_DP_INTERRUPT,
    METRIC_SP_INTERRUPT,
    METRIC_BLOCK_SYSCONFIG_MISS,
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

INLINE void set_metric(metric_t metric, uint64_t value) {
    n64_metric_data[metric] = value;
}

INLINE void reset_all_metrics() {
    for (int i = 0; i < NUM_METRICS; i++) {
        n64_metric_data[i] = 0;
    }
}

#ifdef __cplusplus
}
#endif

#endif //N64_METRICS_H
