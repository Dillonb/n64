#ifndef N64_SCHEDULER_H
#define N64_SCHEDULER_H

#include <util.h>
#include <stdbool.h>

typedef enum scheduler_event_type {
    SCHEDULER_SI_DMA_COMPLETE
} scheduler_event_type_t;

typedef struct scheduler_event {
    dword time;
    scheduler_event_type_t type;
} scheduler_event_t;


void scheduler_reset();
bool scheduler_tick(dword cycles, scheduler_event_t* event);
void scheduler_enqueue_absolute(dword at_cycles, scheduler_event_type_t event_type);
void scheduler_enqueue_relative(dword in_cycles, scheduler_event_type_t event_type);

#endif //N64_SCHEDULER_H
