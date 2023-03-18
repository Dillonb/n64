#include <stdlib.h>
#include <log.h>
#include "scheduler.h"

scheduler_t n64scheduler;

// pops a pointer off of the free event nodes stack
scheduler_event_node_t* alloc_event_node() {
    if (n64scheduler.free_event_nodes_stack_ptr == 0) {
        logfatal("Ran out of free scheduler event nodes!");
    }
    return n64scheduler.free_event_nodes[--n64scheduler.free_event_nodes_stack_ptr];
}

// pushes this pointer onto the free event nodes stack
void free_event_node(scheduler_event_node_t* node) {
    n64scheduler.free_event_nodes[n64scheduler.free_event_nodes_stack_ptr++] = node;
}

void scheduler_reset() {
    n64scheduler.scheduler_ticks = 0;
    n64scheduler.free_event_nodes_stack_ptr = 0;
    n64scheduler.scheduler_list = NULL;

    for (int i = 0; i < NUM_EVENT_NODES; i++) {
        free_event_node(&n64scheduler.event_nodes[i]);
    }
}

bool scheduler_tick(u64 ticks, scheduler_event_t* event) {
    n64scheduler.scheduler_ticks += ticks;

    bool event_occurred = (n64scheduler.scheduler_list != NULL) && n64scheduler.scheduler_list->event.time < n64scheduler.scheduler_ticks;

    if (event_occurred) {
        *event = n64scheduler.scheduler_list->event;
        free_event_node(n64scheduler.scheduler_list);
        n64scheduler.scheduler_list = n64scheduler.scheduler_list->next;
    }

    return event_occurred;
}

void scheduler_enqueue_absolute(u64 at_ticks, scheduler_event_type_t event_type) {
    scheduler_event_node_t* ins = alloc_event_node();
    ins->next = NULL;
    ins->event.type = event_type;
    ins->event.time = at_ticks;

    // special case when list is empty
    if (n64scheduler.scheduler_list == NULL) {
        n64scheduler.scheduler_list = ins;
    } else {
        // Find the first node with a rank smaller than the node we're inserting
        // that either has a null next node OR a next node with a larger rank than the one we're inserting.

        scheduler_event_node_t* n = n64scheduler.scheduler_list;
        while (n->next != NULL && n->next->event.time < at_ticks) {
            n = n->next;
        }
        scheduler_event_node_t* old_next = n->next;
        n->next = ins;
        ins->next = old_next;
    }
}

void scheduler_enqueue_relative(u64 in_ticks, scheduler_event_type_t event_type) {
    scheduler_enqueue_absolute(n64scheduler.scheduler_ticks + in_ticks, event_type);
}

u64 scheduler_remove_event(scheduler_event_type_t event_type) {
    scheduler_event_node_t* node = n64scheduler.scheduler_list;
    scheduler_event_node_t** prev_next = &n64scheduler.scheduler_list;

    while (node != NULL) {
        if (node->event.type == event_type) {
            u64 in_cycles = node->event.time - n64scheduler.scheduler_ticks;
            free_event_node(node);

            *prev_next = node->next;
            return in_cycles;
        }

        prev_next = &node->next;
        node = node->next;
    }
    return 0;
}