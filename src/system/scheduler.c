#include <stdlib.h>
#include <log.h>
#include "scheduler.h"

#define NUM_EVENT_NODES 10

dword scheduler_ticks = 0;

typedef struct scheduler_event_node {
    scheduler_event_t event;
    struct scheduler_event_node* next;
} scheduler_event_node_t;

scheduler_event_node_t event_nodes[NUM_EVENT_NODES];
int free_event_nodes_stack_ptr = 0;
scheduler_event_node_t* free_event_nodes[NUM_EVENT_NODES];

scheduler_event_node_t* scheduler_list = NULL;

// pops a pointer off of the free event nodes stack
scheduler_event_node_t* alloc_event_node() {
    return free_event_nodes[--free_event_nodes_stack_ptr];
}

// pushes this pointer onto the free event nodes stack
void free_event_node(scheduler_event_node_t* node) {
    free_event_nodes[free_event_nodes_stack_ptr++] = node;
}

void scheduler_reset() {
    scheduler_ticks = 0;
    free_event_nodes_stack_ptr = 0;

    for (int i = 0; i < NUM_EVENT_NODES; i++) {
        free_event_node(&event_nodes[i]);
    }
}

bool scheduler_tick(dword ticks, scheduler_event_t* event) {
    scheduler_ticks += ticks;

    bool event_occurred = (scheduler_list != NULL) && scheduler_list->event.time < scheduler_ticks;

    if (event_occurred) {
        *event = scheduler_list->event;
        free_event_node(scheduler_list);
        scheduler_list = scheduler_list->next;
    }

    return event_occurred;
}

void scheduler_enqueue_absolute(dword at_ticks, scheduler_event_type_t event_type) {
    scheduler_event_node_t* ins = alloc_event_node();
    ins->next = NULL;
    ins->event.type = event_type;
    ins->event.time = at_ticks;

    // special case when list is empty
    if (scheduler_list == NULL) {
        scheduler_list = ins;
    } else {
        logfatal("Scheduling more than one event not yet implemented");
    }
}

void scheduler_enqueue_relative(dword in_ticks, scheduler_event_type_t event_type) {
    scheduler_enqueue_absolute(scheduler_ticks + in_ticks, event_type);
}
