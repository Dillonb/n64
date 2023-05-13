#include <r4300i.h>
#include "scheduler_utils.h"
#include "scheduler.h"

void reschedule_compare_interrupt(u32 index) {
    u64 resolved_count = N64CP0.count + index;
    u32 count_shifted = resolved_count >> 1;

    u64 in_cycles = (N64CP0.compare - count_shifted) << 1;
    scheduler_remove_event(SCHEDULER_COMPARE_INTERRUPT);
    scheduler_enqueue_relative(in_cycles, SCHEDULER_COMPARE_INTERRUPT);
}