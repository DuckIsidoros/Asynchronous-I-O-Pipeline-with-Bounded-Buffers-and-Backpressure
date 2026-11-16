#include "bounded_queue.h"
#include <stdlib.h>
#include <assert.h>
#include <time.h>
static inline void assert_inv1(const bounded_queue_t *q)
{
    assert(q->count <= q->capacity);
}
    static uint64_t elapsed_ns(
       const struct timespec *start,
       const struct timespec *end)
    {
        int64_t seconds =
            (int64_t)end->tv_sec - (int64_t)start->tv_sec;

        int64_t nanoseconds =
            (int64_t)end->tv_nsec - (int64_t)start->tv_nsec;

        return (uint64_t)(seconds * 1000000000LL + nanoseconds);
    }


int bounded_queue_init(bounded_queue_t *q, size_t capacity)
{
    if (!q || capacity == 0)
        return -1;
    q->slots = malloc(sizeof(pipeline_block_t *) * capacity);
    if (!q->slots)
        return -1;

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->max_occupancy = 0;
    q->producer_stall_ns = 0;
    q->consumer_stall_ns = 0;
    q->is_shutdown = false;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);

    assert_inv1(q);
    return 0;
}

bool bounded_queue_push(bounded_queue_t *q, pipeline_block_t **block_ref)
{
    if (!q || !block_ref || !*block_ref)
        return false;

    pthread_mutex_lock(&q->lock);

    while (q->count == q->capacity && !q->is_shutdown)
    {
       struct timespec start, end;
       clock_gettime(CLOCK_MONOTONIC, &start);
       pthread_cond_wait(&q->not_full, &q->lock);
       clock_gettime(CLOCK_MONOTONIC, &end);
       q->producer_stall_ns += elapsed_ns(&start, &end);
    }

    if (q->is_shutdown)
    {
        pthread_mutex_unlock(&q->lock);
        return false;
    }

    assert_inv1(q);
    q->slots[q->tail] = *block_ref;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    if (q->count > q->max_occupancy)
        {
            q->max_occupancy = q->count;
        }

    *block_ref = NULL;
    assert_inv1(q);

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return true;
}

bool bounded_queue_pop(bounded_queue_t *q, pipeline_block_t **out_block)
{
    if (!q || !out_block)
        return false;

    pthread_mutex_lock(&q->lock);

    while (q->count == 0 && !q->is_shutdown)
    {
       struct timespec start, end;
       clock_gettime(CLOCK_MONOTONIC, &start);
       pthread_cond_wait(&q->not_empty, &q->lock);
       clock_gettime(CLOCK_MONOTONIC, &end);
       q->consumer_stall_ns += elapsed_ns(&start, &end);
    }

    if (q->count == 0 && q->is_shutdown)
    {
        pthread_mutex_unlock(&q->lock);
        *out_block = NULL;
        return false;
    }

    assert_inv1(q);



    *out_block = q->slots[q->head];
    q->slots[q->head] = NULL;
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    assert_inv1(q);

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return true;
}

void bounded_queue_shutdown(bounded_queue_t *q)
{
    if (!q)
        return;

    pthread_mutex_lock(&q->lock);
    q->is_shutdown = true;
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

void bounded_queue_destroy(bounded_queue_t *q)
{
    if (!q)
        return;

    if (q->slots)
    {
        for (size_t i = 0; i < q->capacity; ++i)
        {
            if (q->slots[i])
            {
                if (q->slots[i]->data)
                {
                    free(q->slots[i]->data);
                    q->slots[i]->data = NULL;
                }
                free(q->slots[i]);
                q->slots[i] = NULL;
            }
        }
        free(q->slots);
        q->slots = NULL;
    }

    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
}
bool bounded_queue_get_telemetry(
    bounded_queue_t *q,
    queue_telemetry_t *snapshot)
{
    if (!q || !snapshot)
        return false;

    pthread_mutex_lock(&q->lock);

    snapshot->current_occupancy = q->count;
    snapshot->max_occupancy = q->max_occupancy;
    snapshot->capacity = q->capacity;
    snapshot->producer_stall_ns = q->producer_stall_ns;
    snapshot->consumer_stall_ns = q->consumer_stall_ns;

    pthread_mutex_unlock(&q->lock);

    return true;
}
