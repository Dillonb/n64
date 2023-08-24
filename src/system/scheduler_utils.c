#include <r4300i.h>
#include "scheduler_utils.h"
#include "scheduler.h"

void reschedule_compare_interrupt(u32 index) {
    u64 resolved_count = N64CP0.count + index;
    u32 compare_shifted = N64CP0.compare << 1;

    u64 in_cycles = (compare_shifted - resolved_count) & 0x1FFFFFFFF;
    scheduler_remove_event(SCHEDULER_COMPARE_INTERRUPT);
    scheduler_enqueue_relative(in_cycles, SCHEDULER_COMPARE_INTERRUPT);
}