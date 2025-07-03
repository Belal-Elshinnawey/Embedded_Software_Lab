#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    float com_x;
    float com_y;
} COM;

#define COM_QUEUE_SIZE 256

typedef struct {
    COM buffer[COM_QUEUE_SIZE];
    atomic_size_t head;
    atomic_size_t tail;
} SPSCQueue;

static inline void spsc_init(SPSCQueue *q) {
    atomic_init(&q->head, 0);
    atomic_init(&q->tail, 0);
}

static inline bool spsc_enqueue(SPSCQueue *q, COM value) {
    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    size_t next_head = (head + 1) & (COM_QUEUE_SIZE - 1);

    if (next_head == tail) {
        return false;
    }

    q->buffer[head] = value;
    atomic_store_explicit(&q->head, next_head, memory_order_release);
    return true;
}

static inline bool spsc_dequeue(SPSCQueue *q, COM *value) {
    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    if (head == tail) {
        return false; 
    }
    *value = q->buffer[tail];
    size_t next_tail = (tail + 1) & (COM_QUEUE_SIZE - 1);
    atomic_store_explicit(&q->tail, next_tail, memory_order_release);
    return true;
}

#endif
