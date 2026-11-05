#include "common.h"
#include "bounded_queue.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

// Define worker thread context structure.
typedef struct worker_ctx
{
    bounded_queue_t *in_queue;
    bounded_queue_t *out_queue;
    void (*transform)(chunk_t *chunk);
} worker_ctx_t;

// Default transformation function that can be used if no custom callback is supplied.
static void default_transform(chunk_t *chunk)
{
    // Validate chunk state prior to in-place processing.
    assert_chunk_valid(chunk);
}

// Worker thread entry point executing the input-transform-output loop.
void *worker_thread(void *arg)
{
    // Cast argument to worker context.
    worker_ctx_t *ctx = (worker_ctx_t *)arg;

    // Validate that context and queues are non-null.
    assert(ctx != NULL && ctx->in_queue != NULL && ctx->out_queue != NULL);

    // Initialize local chunk pointer to NULL.
    chunk_t *chunk = NULL;

    // Main worker loop processing chunks until EOF or queue shutdown.
    while (bounded_queue_pop(ctx->in_queue, (pipeline_block_t **)&chunk))
    {
        // Assert chunk validity after acquisition from input queue.
        assert_chunk_valid(chunk);

        // Check for EOF sentinel to handle stream teardown.
        if (chunk->is_eof)
        {
            // Propagate EOF sentinel to output queue and enforce Inv4 ownership transfer.
            bounded_queue_push(ctx->out_queue, (pipeline_block_t **)&chunk);

            // Assert that local ownership was surrendered.
            assert_single_ownership((const chunk_t *const *)&chunk);

            // Break from worker loop upon sentinel propagation.
            break;
        }

        // Apply payload transformation.
        if (ctx->transform)
        {
            ctx->transform(chunk);
        }
        else
        {
            default_transform(chunk);
        }

        // Validate chunk state invariants after transformation.
        assert_chunk_valid(chunk);

        // Transfer transformed chunk ownership to output queue.
        bounded_queue_push(ctx->out_queue, (pipeline_block_t **)&chunk);

        // Verify single-ownership invariant that caller reference is now null.
        assert_single_ownership((const chunk_t *const *)&chunk);
    }

    // Clean exit of worker thread.
    return NULL;
}

// Alias worker routine for compatibility with alternative naming conventions.
void *worker_routine(void *arg)
{
    // Delegate to worker_thread implementation.
    return worker_thread(arg);
}
