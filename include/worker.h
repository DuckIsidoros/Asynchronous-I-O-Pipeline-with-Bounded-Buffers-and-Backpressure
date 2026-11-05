#ifndef WORKER_H
#define WORKER_H

#include "common.h"
#include "bounded_queue.h"

// Define worker thread context structure for pipeline stages.
typedef struct worker_ctx
{
    bounded_queue_t *in_queue;
    bounded_queue_t *out_queue;
    void (*transform)(chunk_t *chunk);
} worker_ctx_t;

// Worker thread entry point executing the input-transform-output loop.
void *worker_thread(void *arg);

// Alias worker routine for compatibility with alternative thread conventions.
void *worker_routine(void *arg);

#endif
