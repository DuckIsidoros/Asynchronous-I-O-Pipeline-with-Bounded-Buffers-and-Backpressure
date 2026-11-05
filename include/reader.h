#ifndef READER_H
#define READER_H

#include "common.h"
#include "bounded_queue.h"

// Define Reader thread context structure.
typedef struct reader_ctx
{
    int fd_in;
    size_t chunk_size;
    bounded_queue_t *out_queue;
    size_t num_workers;
} reader_ctx_t;

// Reader thread entry point reading from fd_in and feeding the pipeline.
void *reader_thread(void *arg);

#endif
