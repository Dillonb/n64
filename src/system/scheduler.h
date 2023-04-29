#ifndef N64_SCHEDULER_H
#define N64_SCHEDULER_H

#include <util.h>
#include <stdbool.h>

typedef enum scheduler_event_type {
    SCHEDULER_SI_DMA_COMPLETE,
    SCHEDULER_PI_DMA_COMPLETE,
    SCHEDULER_PI_BUS_WRITE_COMPLETE,
    SCHEDULER_VI_HALFLINE,
    SCHEDULER_RESET_SYSTEM
} scheduler_event_type_t;

typedef struct scheduler_event {
    u64 time;
    scheduler_event_type_t type;
} scheduler_event_t;

#define NUM_EVENT_NODES 10
typedef struct scheduler_event_node {
    scheduler_event_t event;
    struct scheduler_event_node* next;
} scheduler_event_node_t;

typedef struct scheduler {
    u64 scheduler_ticks;
    scheduler_event_node_t event_nodes[NUM_EVENT_NODES];
    int free_event_nodes_stack_ptr;
    scheduler_event_node_t* free_event_nodes[NUM_EVENT_NODES];
    scheduler_event_node_t* scheduler_list;
} scheduler_t;

extern scheduler_t n64scheduler;

void scheduler_reset();
bool scheduler_tick(u64 cycles, scheduler_event_t* event);
u64 scheduler_remove_event(scheduler_event_type_t event_type);
void scheduler_enqueue_absolute(u64 at_cycles, scheduler_event_type_t event_type);
void scheduler_enqueue_relative(u64 in_cycles, scheduler_event_type_t event_type);

#endif //N64_SCHEDULER_H
