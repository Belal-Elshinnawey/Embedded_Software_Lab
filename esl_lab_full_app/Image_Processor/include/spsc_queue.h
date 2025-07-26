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

static inline bool spsc_enqueue(SPSCQueue *q, COM value) { //write function
    //read the head, only load operation is atomic. Allow reordering since the writer is the own thread.
    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    //read the tail with acquire. The consumer will load the tail, then
    // write to buffer, then release the tail. Make sure that tail release 
    // and buffer writes prior are visable to the producer before reading the tail
    size_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    // The head will never be stale here becuase the writing thread cannot fire
    // while its last firing is goning on, that means relaxed is safe.
    size_t next_head = (head + 1) & (COM_QUEUE_SIZE - 1); 
    if (next_head == tail) {// queue is full.
        return false;
    }
    // push the value to the buffer. The consumer reads from tail and tail is not head.
    q->buffer[head] = value;
    // store the head value. use release so head change is visable to the consumer
    atomic_store_explicit(&q->head, next_head, memory_order_release);
    return true;
}

static inline bool spsc_dequeue(SPSCQueue *q, COM *value) {
    // same story, now tail is only changed by the consumer, so its relaxed
    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    // head changes and prior writes need to be visable prior to the writes
    size_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    if (head == tail) { // queue is empty.
        return false; 
    }
    *value = q->buffer[tail];//read teh value
    size_t next_tail = (tail + 1) & (COM_QUEUE_SIZE - 1); //update the tail
    atomic_store_explicit(&q->tail, next_tail, memory_order_release);//use release
    return true;
}

#endif
