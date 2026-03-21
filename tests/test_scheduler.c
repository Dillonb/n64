#include <string.h>
#include <system/scheduler.h>

#define SHOULD_LOG_PASSED_TESTS false
#include "unit.h"

void test_enqueue_and_fire_single_event() {
    scheduler_reset();

    scheduler_event_t event;
    scheduler_enqueue_relative(5, SCHEDULER_VI_HALFLINE);

    // Should not fire before the scheduled time
    ASSERT_TRUE(!scheduler_tick(4, &event),
        "single event: should not fire before scheduled time");

    // Should fire exactly at the scheduled time
    ASSERT_TRUE(scheduler_tick(1, &event),
        "single event: should fire at scheduled time");
    ASSERT_EQ(event.type, SCHEDULER_VI_HALFLINE,
        "single event: event type should match");

    // No more events
    ASSERT_TRUE(!scheduler_tick(1, &event),
        "single event: no event after all consumed");
}

void test_event_fires_at_exact_tick() {
    scheduler_reset();

    // Schedule event at absolute tick 10
    scheduler_enqueue_absolute(10, SCHEDULER_SI_DMA_COMPLETE);

    scheduler_event_t event;

    // Advance to tick 9
    ASSERT_FALSE(scheduler_tick(9, &event),
        "exact tick: should not fire at tick 9");

    // Advance to tick 10 — should fire
    ASSERT_TRUE(scheduler_tick(1, &event),
        "exact tick: should fire at tick 10");
    ASSERT_EQ(event.type, SCHEDULER_SI_DMA_COMPLETE,
        "exact tick: event type should match");
}

void test_event_fires_when_overshooting() {
    scheduler_reset();

    scheduler_enqueue_relative(3, SCHEDULER_PI_DMA_COMPLETE);

    scheduler_event_t event;

    // Advance past the event in one big step
    ASSERT_TRUE(scheduler_tick(10, &event),
        "overshoot: event should fire when ticks jump past it");
    ASSERT_EQ(event.type, SCHEDULER_PI_DMA_COMPLETE,
        "overshoot: event type should match");
}

void test_multiple_events_fire_in_order() {
    scheduler_reset();

    scheduler_enqueue_relative(10, SCHEDULER_PI_DMA_COMPLETE);
    scheduler_enqueue_relative(5, SCHEDULER_VI_HALFLINE);
    scheduler_enqueue_relative(15, SCHEDULER_SI_DMA_COMPLETE);

    scheduler_event_t event;

    // First event at tick 5
    ASSERT_TRUE(!scheduler_tick(4, &event),
        "ordering: no event at tick 4");
    ASSERT_TRUE(scheduler_tick(1, &event),
        "ordering: first event fires at tick 5");
    ASSERT_EQ(event.type, SCHEDULER_VI_HALFLINE,
        "ordering: first event is VI_HALFLINE");

    // Second event at tick 10
    ASSERT_TRUE(!scheduler_tick(4, &event),
        "ordering: no event at tick 9");
    ASSERT_TRUE(scheduler_tick(1, &event),
        "ordering: second event fires at tick 10");
    ASSERT_EQ(event.type, SCHEDULER_PI_DMA_COMPLETE,
        "ordering: second event is PI_DMA_COMPLETE");

    // Third event at tick 15
    ASSERT_TRUE(!scheduler_tick(4, &event),
        "ordering: no event at tick 14");
    ASSERT_TRUE(scheduler_tick(1, &event),
        "ordering: third event fires at tick 15");
    ASSERT_EQ(event.type, SCHEDULER_SI_DMA_COMPLETE,
        "ordering: third event is SI_DMA_COMPLETE");
}

void test_ticks_until_next_event() {
    scheduler_reset();

    scheduler_enqueue_relative(7, SCHEDULER_VI_HALFLINE);

    ASSERT_EQ(scheduler_ticks_until_next_event(), 7,
        "ticks_until: should be 7 initially");

    scheduler_event_t event;
    scheduler_tick(3, &event);

    ASSERT_EQ(scheduler_ticks_until_next_event(), 4,
        "ticks_until: should be 4 after advancing 3");
}

void test_ticks_until_matches_fire_time() {
    scheduler_reset();

    scheduler_enqueue_relative(5, SCHEDULER_VI_HALFLINE);

    u64 ticks_until = scheduler_ticks_until_next_event();
    ASSERT_EQ(ticks_until, 5,
        "ticks_matches_fire: ticks_until should be 5");

    scheduler_event_t event;
    ASSERT_TRUE(scheduler_tick(ticks_until, &event),
        "ticks_matches_fire: event should fire when advancing by ticks_until");
    ASSERT_EQ(event.type, SCHEDULER_VI_HALFLINE,
        "ticks_matches_fire: event type should match");
}

void test_remove_event() {
    scheduler_reset();

    scheduler_enqueue_relative(5, SCHEDULER_VI_HALFLINE);
    scheduler_enqueue_relative(10, SCHEDULER_PI_DMA_COMPLETE);

    u64 remaining = scheduler_remove_event(SCHEDULER_VI_HALFLINE);
    ASSERT_EQ(remaining, 5,
        "remove: should return remaining ticks");

    // The removed event should not fire
    scheduler_event_t event;
    ASSERT_TRUE(!scheduler_tick(5, &event),
        "remove: removed event should not fire");

    // The other event should still fire
    ASSERT_TRUE(scheduler_tick(5, &event),
        "remove: remaining event should fire");
    ASSERT_EQ(event.type, SCHEDULER_PI_DMA_COMPLETE,
        "remove: remaining event type should match");
}

void test_remove_nonexistent_event() {
    scheduler_reset();

    scheduler_enqueue_relative(5, SCHEDULER_VI_HALFLINE);

    u64 remaining = scheduler_remove_event(SCHEDULER_PI_DMA_COMPLETE);
    ASSERT_EQ(remaining, 0,
        "remove_nonexistent: should return 0 for missing event");

    // Original event should still fire
    scheduler_event_t event;
    ASSERT_TRUE(scheduler_tick(5, &event),
        "remove_nonexistent: original event should still fire");
}

void test_enqueue_at_same_time() {
    scheduler_reset();

    scheduler_enqueue_relative(5, SCHEDULER_VI_HALFLINE);
    scheduler_enqueue_relative(5, SCHEDULER_PI_DMA_COMPLETE);

    scheduler_event_t event;

    // First event at tick 5
    ASSERT_TRUE(scheduler_tick(5, &event),
        "same_time: first event fires at tick 5");

    // Second event also at tick 5 — should fire on next tick call
    ASSERT_TRUE(scheduler_tick(0, &event),
        "same_time: second event fires at same tick");
}

void test_enqueue_relative_zero() {
    scheduler_reset();

    scheduler_enqueue_relative(0, SCHEDULER_RESET_SYSTEM);

    scheduler_event_t event;

    // Event at current tick — should fire on next scheduler_tick with 0 advance
    ASSERT_TRUE(scheduler_tick(0, &event),
        "relative_zero: event scheduled at 0 should fire immediately");
    ASSERT_EQ(event.type, SCHEDULER_RESET_SYSTEM,
        "relative_zero: event type should match");
}

void test_absolute_enqueue() {
    scheduler_reset();

    // Advance to tick 100
    scheduler_event_t event;
    scheduler_tick(100, &event);

    // Schedule at absolute tick 150
    scheduler_enqueue_absolute(150, SCHEDULER_COMPARE_INTERRUPT);

    ASSERT_EQ(scheduler_ticks_until_next_event(), 50,
        "absolute: ticks_until should be 50");

    ASSERT_TRUE(!scheduler_tick(49, &event),
        "absolute: should not fire at tick 149");
    ASSERT_TRUE(scheduler_tick(1, &event),
        "absolute: should fire at tick 150");
    ASSERT_EQ(event.type, SCHEDULER_COMPARE_INTERRUPT,
        "absolute: event type should match");
}

void test_one_event_per_tick_call() {
    // scheduler_tick returns only one event at a time
    scheduler_reset();

    scheduler_enqueue_relative(5, SCHEDULER_VI_HALFLINE);
    scheduler_enqueue_relative(5, SCHEDULER_PI_DMA_COMPLETE);

    scheduler_event_t event;

    // Jump to tick 5 — should get first event
    ASSERT_TRUE(scheduler_tick(5, &event),
        "one_per_call: first tick call returns first event");

    // Call again with 0 ticks — should get second event
    ASSERT_TRUE(scheduler_tick(0, &event),
        "one_per_call: second tick call returns second event");

    // No more events
    ASSERT_TRUE(!scheduler_tick(0, &event),
        "one_per_call: third tick call returns no event");
}

int main() {
    test_enqueue_and_fire_single_event();
    test_event_fires_at_exact_tick();
    test_event_fires_when_overshooting();
    test_multiple_events_fire_in_order();
    test_ticks_until_next_event();
    test_ticks_until_matches_fire_time();
    test_remove_event();
    test_remove_nonexistent_event();
    test_enqueue_at_same_time();
    test_enqueue_relative_zero();
    test_absolute_enqueue();
    test_one_event_per_tick_call();

    printf("\n");
    if (tests_failed > 0) {
        printf(COLOR_RED "%d test(s) failed\n" COLOR_END, tests_failed);
    } else {
        printf(COLOR_GREEN "All tests passed\n" COLOR_END);
    }
    return tests_failed > 0 ? 1 : 0;
}
