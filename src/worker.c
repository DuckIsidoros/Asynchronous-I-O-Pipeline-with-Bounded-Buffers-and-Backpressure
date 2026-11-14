#include "common.h"
#include "bounded_queue.h"
#include "worker.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

static void default_transform(chunk_t *chunk)
{
    assert_chunk_valid(chunk);
}

void *worker_thread(void *arg)
{
    worker_ctx_t *ctx = (worker_ctx_t *)arg;
    assert(ctx != NULL && ctx->in_queue != NULL && ctx->out_queue != NULL);

    chunk_t *chunk = NULL;

    while (bounded_queue_pop(ctx->in_queue, (pipeline_block_t **)&chunk))
    {
        assert_chunk_valid(chunk);

        if (chunk->is_eof)
        {
            if (!bounded_queue_push(ctx->out_queue, (pipeline_block_t **)&chunk))
            {
                goto cleanup;
            }
            assert_single_ownership((const chunk_t *const *)&chunk);
            break;
        }

        if (ctx->transform)
        {
            ctx->transform(chunk);
        }
        else
        {
            default_transform(chunk);
        }

        assert_chunk_valid(chunk);

        if (!bounded_queue_push(ctx->out_queue, (pipeline_block_t **)&chunk))
        {
            goto cleanup;
        }

        assert_single_ownership((const chunk_t *const *)&chunk);
    }

    return NULL;

cleanup:
    bounded_queue_shutdown(ctx->in_queue);
    bounded_queue_shutdown(ctx->out_queue);
    if (chunk)
    {
        chunk_destroy(&chunk);
    }
    return NULL;
}

void *worker_routine(void *arg)
{
    return worker_thread(arg);
}
