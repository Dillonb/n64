#ifndef N64_RING_BUFFER_H
#define N64_RING_BUFFER_H

#include <stdatomic.h>
#include <util.h>
#include <SDL.h>

typedef struct ring_buffer {
    int capacity;
    volatile unsigned size;
    volatile unsigned write_idx;
    volatile unsigned read_idx;
    volatile bool full;
    float* data;
    SDL_mutex* mutex;
} ring_buffer_t;

INLINE void ring_buffer_init(ring_buffer_t* buf, uint capacity) {
    buf->capacity = capacity;
    buf->data = malloc(capacity * sizeof(float));
    buf->size = 0;
    buf->full = false;

    buf->write_idx = 0;
    buf->read_idx = 0;
    buf->mutex = SDL_CreateMutex();
}

INLINE void ring_buffer_push_blocking(ring_buffer_t* buf, float val) {
    while (buf->full) {}
    SDL_LockMutex(buf->mutex);
    buf->data[buf->write_idx] = val;
    buf->write_idx++;
    if (buf->write_idx >= buf->capacity) {
        buf->write_idx = 0;
    }
    buf->size++;
    buf->full = buf->size == buf->capacity;
    SDL_UnlockMutex(buf->mutex);
}

INLINE float ring_buffer_pop(ring_buffer_t* buf) {
    if (buf->size == 0) {
        return 0;
    } else {
        SDL_LockMutex(buf->mutex);
        float val = buf->data[buf->read_idx];
        buf->read_idx++;
        if (buf->read_idx >= buf->capacity) {
            buf->read_idx = 0;
        }
        buf->size--;
        buf->full = buf->size == buf->capacity;
        SDL_UnlockMutex(buf->mutex);
        return val;
    }
}

#endif //N64_RING_BUFFER_H
