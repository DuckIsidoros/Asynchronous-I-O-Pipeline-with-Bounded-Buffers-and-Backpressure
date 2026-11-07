#include "reader.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

// Reader thread entry point executing the raw ingestion and packaging loop.
void *reader_thread(void *arg)
{
    // Cast argument to reader context and validate parameters.
    reader_ctx_t *ctx = (reader_ctx_t *)arg;
    assert(ctx != NULL && ctx->out_queue != NULL && ctx->chunk_size > 0 && ctx->num_workers > 0);

    // Initialize sequence counter for strict total ordering.
    uint64_t seq_id = 0;

    // Loop reading input data blocks until EOF is encountered.
    while (1)
    {
        // Allocate next data chunk for ingestion.
        chunk_t *chunk = chunk_create(ctx->chunk_size, seq_id);
        assert(chunk != NULL);

        // Track accumulated bytes read for this chunk.
        size_t bytes_read = 0;
        bool eof_encountered = false;

        // Execute robust read loop handling short reads and EINTR signals (FR-4).
        while (bytes_read < ctx->chunk_size)
        {
            // Perform read system call on input descriptor.
            ssize_t n = read(ctx->fd_in, chunk->data + bytes_read, ctx->chunk_size - bytes_read);
            if (n > 0)
            {
                // Advance buffer offset by transferred byte count.
                bytes_read += (size_t)n;
            }
            else if (n == 0)
            {
                // Detect stream EOF cleanly.
                eof_encountered = true;
                break;
            }
            else
            {
                // Retry immediately on EINTR; abort read loop on fatal errors.
                if (errno == EINTR)
                {
                    continue;
                }
                perror("reader_thread: read error");
                eof_encountered = true;
                break;
            }
        }

        // Push valid data chunk to queue or discard empty buffer.
        if (bytes_read > 0)
        {
            // Set valid payload size and advance sequence identifier.
            chunk->size = bytes_read;
            seq_id++;
            assert_chunk_valid(chunk);

            // Transfer ownership to downstream queue.
            if (!bounded_queue_push(ctx->out_queue, (pipeline_block_t **)&chunk))
            {
                // Destroy chunk on rejected push during shutdown.
                chunk_destroy(&chunk);
                break;
            }

            // Assert that caller surrendered ownership (Inv4).
            assert_single_ownership((const chunk_t *const *)&chunk);
        }
        else
        {
            // Release unused empty chunk when EOF is hit at chunk boundary.
            chunk_destroy(&chunk);
        }

        // Terminate read loop when input stream terminates.
        if (eof_encountered)
        {
            break;
        }
    }

    // Propagate exactly num_workers EOF sentinels for worker shutdown (FR-1, FR-5).
    for (size_t i = 0; i < ctx->num_workers; ++i)
    {
        // Allocate EOF sentinel chunk carrying monotonic sequence id.
        chunk_t *sentinel = chunk_create_sentinel(seq_id++);
        assert(sentinel != NULL);

        // Push sentinel into queue to terminate one downstream worker.
        if (!bounded_queue_push(ctx->out_queue, (pipeline_block_t **)&sentinel))
        {
            // Destroy sentinel if queue is shut down.
            chunk_destroy(&sentinel);
            break;
        }

        // Verify single ownership transfer of sentinel chunk.
        assert_single_ownership((const chunk_t *const *)&sentinel);
    }

    // Exit reader thread cleanly.
    return NULL;
}
