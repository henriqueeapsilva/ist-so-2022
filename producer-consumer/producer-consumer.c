#include <stdio.h>
#include "producer-consumer.h"
#include <stdlib.h>

// pcq_create: create a queue, with a given (fixed) capacity
//
// Memory: the queue pointer must be previously allocated
// (either on the stack or the heap)
int pcq_create(pc_queue_t *queue, size_t capacity){

    // Allocate memory for pcq_buffer
    queue->pcq_buffer = calloc(capacity, sizeof(void*));
    if(queue->pcq_buffer == NULL) return -1;

    // initializations
    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;

    pthread_mutex_init(&queue->pcq_current_size_lock, NULL);
    pthread_mutex_init(&queue->pcq_head_lock, NULL);
    pthread_mutex_init(&queue->pcq_tail_lock, NULL);
    pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL);
    pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL);

    pthread_cond_init(&queue->pcq_pusher_condvar, NULL);
    pthread_cond_init(&queue->pcq_popper_condvar, NULL);
    return 0;
}

// pcq_destroy: releases the internal resources of the queue
//
// Memory: does not free the queue pointer itself
int pcq_destroy(pc_queue_t *queue){

    // free buffer memory
    free(queue->pcq_buffer);

    // destroy locks and condition variables
    pthread_mutex_destroy(&queue->pcq_current_size_lock);
    pthread_mutex_destroy(&queue->pcq_head_lock);
    pthread_mutex_destroy(&queue->pcq_tail_lock);
    pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
    pthread_cond_destroy(&queue->pcq_pusher_condvar);
    pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
    pthread_cond_destroy(&queue->pcq_popper_condvar);

    return 0;
}

// pcq_enqueue: insert a new element at the front of the queue
//
// If the queue is full, sleep until the queue has space
int pcq_enqueue(pc_queue_t *queue, void *elem)
{
    // Acquire lock for current size
    pthread_mutex_lock(&queue->pcq_current_size_lock);

    // Wait while the queue is full
    while (queue->pcq_current_size == queue->pcq_capacity) {
        pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_pusher_condvar_lock);
    }

    // Acquire lock for tail
    pthread_mutex_lock(&queue->pcq_tail_lock);

    // Insert element at tail
    queue->pcq_buffer[queue->pcq_tail] = elem;
    queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;
    queue->pcq_current_size++;

    // Release lock for tail
    pthread_mutex_unlock(&queue->pcq_tail_lock);

    // Release lock for current size
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

    // Signal any waiting popper thread
    pthread_cond_signal(&queue->pcq_popper_condvar);

    return 0;
}


// pcq_dequeue: remove an element from the back of the queue
//
// If the queue is empty, sleep until the queue has an element
void *pcq_dequeue(pc_queue_t *queue)
{
    // Acquire lock for current size
    pthread_mutex_lock(&queue->pcq_current_size_lock);

    // Wait while the queue is empty
    while (queue->pcq_current_size == 0) {
        pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_popper_condvar_lock);
    }

    // Acquire lock for head
    pthread_mutex_lock(&queue->pcq_head_lock);

    // Get element at head
    void *elem = queue->pcq_buffer[queue->pcq_head];
    queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
    queue->pcq_current_size--;

    // Release lock for head
    pthread_mutex_unlock(&queue->pcq_head_lock);

    // Release lock for current size
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

    // Signal any waiting pusher thread
    pthread_cond_signal(&queue->pcq_pusher_condvar);

    return elem;
}

