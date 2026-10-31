#ifndef BOUNDED_QUEUE_H
#define BOUNDED_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct
{
    uint64_t sequence_id;
    size_t payload_len;
    uint8_t *data;
} pipeline_block_t;

typedef struct
{
    pipeline_block_t **slots;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    bool is_shutdown;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} bounded_queue_t;

int bounded_queue_init(bounded_queue_t *q, size_t capacity);
void bounded_queue_destroy(bounded_queue_t *q);
bool bounded_queue_push(bounded_queue_t *q, pipeline_block_t **block_ref);
bool bounded_queue_pop(bounded_queue_t *q, pipeline_block_t **out_block);
void bounded_queue_shutdown(bounded_queue_t *q);

#endif