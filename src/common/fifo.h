#ifndef N64_FIFO_H
#define N64_FIFO_H

#include <string.h>
#include <util.h>

struct fifo {
    uint8_t *data;
    int size;
    int mask;
    _Atomic int head;
    _Atomic int tail;
};

void fifo_write(struct fifo *q, const void *buf, int size) {
    if (q->tail + size > q->size) {
        int size_a = q->size - q->tail;
        int size_b = size - size_a;

        memcpy(q->data + q->tail, buf, size_a);
        memcpy(q->data, buf + size_a, size_b);
    } else {
        memcpy(q->data + q->tail, buf, size);
    }

    int new_tail = (q->tail + size) & q->mask;
    q->tail = new_tail;
}

void fifo_read(struct fifo *q, void *buf, int size) {
    if (q->head + size > q->size) {
        int size_a = q->size - q->head;
        int size_b = size - size_a;

        memcpy(buf, q->data + q->head, size_a);
        memcpy(buf + size_a, q->data, size_b);
    } else {
        memcpy(buf, q->data + q->head, size);
    }

    int new_head = (q->head + size) & q->mask;
    q->head = new_head;
}

int fifo_read_available(struct fifo *q) {
    return (q->tail - q->head) & q->mask;
}

int fifo_write_remaining(struct fifo *q) {
    /* can't use last entry, it exists to disambiguate empty and full states */
    return q->mask - fifo_read_available(q);
}

int fifo_size(struct fifo *q) {
    return q->size;
}

void fifo_destroy(struct fifo *q) {
    free(q->data);
    free(q);
}

struct fifo *fifo_create(int size) {
    struct fifo *q = (struct fifo *)calloc(1, sizeof(struct fifo));

    /* round size up to next power of two */
    size = (int)npow2((u32)size);

    q->data = malloc(size);
    q->size = size;
    q->mask = size - 1;

    return q;
}

#endif //N64_FIFO_H
