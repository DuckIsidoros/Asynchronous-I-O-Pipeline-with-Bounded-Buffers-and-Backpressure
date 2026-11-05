#ifndef WRITER_H
#define WRITER_H

#include "common.h"
#include "bounded_queue.h"

// Define Writer thread context structure.
typedef struct writer_ctx
{
    int fd_out;
    bounded_queue_t *in_queue;
    size_t num_workers;
} writer_ctx_t;

// Writer thread entry point consuming from queue and writing to fd_out.
void *writer_thread(void *arg);

#endif
