#include "writer.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Internal node for buffering out-of-order chunks awaiting sequential egress commit.
typedef struct pending_entry
{
    chunk_t *chunk;
    struct pending_entry *next;
} pending_entry_t;

// Write complete chunk payload to file descriptor handling short writes and EINTR (FR-4).
static void write_chunk_payload(int fd, chunk_t *chunk)
{
    // Validate chunk state before initiating I/O.
    assert_chunk_valid(chunk);

    // Track offset of bytes written to output descriptor.
    size_t bytes_written = 0;

    // Execute offset-advancing loop until entire payload is transferred.
    while (bytes_written < chunk->size)
    {
        // Issue write system call for remaining payload slice.
        ssize_t n = write(fd, chunk->data + bytes_written, chunk->size - bytes_written);
        if (n > 0)
        {
            // Advance written byte offset.
            bytes_written += (size_t)n;
        }
        else if (n < 0)
        {
            // Retry immediately on EINTR; log and break on unrecoverable errors.
            if (errno == EINTR)
            {
                continue;
            }
            perror("writer_thread: write error");
            break;
        }
        else
        {
            // Break on zero-byte return from non-advancing descriptor.
            break;
        }
    }
}

// Insert chunk into pending list in ascending order of sequence_id.
static void insert_pending_chunk(pending_entry_t **head, chunk_t *chunk)
{
    // Allocate wrapper node for pending chunk entry.
    pending_entry_t *node = (pending_entry_t *)malloc(sizeof(pending_entry_t));
    assert(node != NULL);
    node->chunk = chunk;
    node->next = NULL;

    // Insert at list head if empty or smaller than head sequence id.
    if (*head == NULL || (*head)->chunk->seq_id > chunk->seq_id)
    {
        node->next = *head;
        *head = node;
        return;
    }

    // Traverse list to locate correct in-order insertion position.
    pending_entry_t *curr = *head;
    while (curr->next != NULL && curr->next->chunk->seq_id < chunk->seq_id)
    {
        curr = curr->next;
    }

    // Splice new entry into sorted linked list.
    node->next = curr->next;
    curr->next = node;
}

// Drain contiguous matching sequence chunks from pending list to output descriptor.
static void drain_contiguous_pending(int fd, pending_entry_t **head, uint64_t *expected_seq_id)
{
    // Flush all contiguous chunks matching expected sequential order.
    while (*head != NULL && (*head)->chunk->seq_id == *expected_seq_id)
    {
        // Unlink head node from pending queue.
        pending_entry_t *curr = *head;
        *head = curr->next;

        // Commit chunk payload to destination descriptor.
        write_chunk_payload(fd, curr->chunk);

        // Advance expected sequence identifier.
        (*expected_seq_id)++;

        // Destroy committed chunk and enforce Inv4 single ownership transfer.
        chunk_destroy(&curr->chunk);
        assert_single_ownership((const chunk_t *const *)&curr->chunk);

        // Free pending tracking node.
        free(curr);
    }
}

// Free all uncommitted pending entries during emergency or shutdown teardown.
static void clear_pending_list(pending_entry_t **head)
{
    // Iterate and release each stranded node and its wrapped chunk.
    while (*head != NULL)
    {
        pending_entry_t *temp = *head;
        *head = (*head)->next;
        chunk_destroy(&temp->chunk);
        free(temp);
    }
}

// Writer thread entry point consuming from queue and performing ordered commits.
void *writer_thread(void *arg)
{
    // Cast argument to writer context and validate inputs.
    writer_ctx_t *ctx = (writer_ctx_t *)arg;
    assert(ctx != NULL && ctx->in_queue != NULL && ctx->num_workers > 0);

    // Initialize expected sequence tracker and sentinel shutdown counter.
    uint64_t expected_seq_id = 0;
    size_t sentinels_received = 0;
    pending_entry_t *pending_head = NULL;

    // Process incoming chunks from queue until all worker sentinels are collected.
    chunk_t *chunk = NULL;
    while (bounded_queue_pop(ctx->in_queue, (pipeline_block_t **)&chunk))
    {
        // Assert chunk state integrity upon dequeue.
        assert_chunk_valid(chunk);

        // Check if popped chunk is an EOF sentinel tombstone.
        if (chunk->is_eof)
        {
            // Increment count of collected worker sentinels (FR-1, FR-5).
            sentinels_received++;

            // Destroy sentinel chunk immediately and zero local pointer (Inv4).
            chunk_destroy(&chunk);
            assert_single_ownership((const chunk_t *const *)&chunk);

            // Break when all workers have completed and pending buffer is drained.
            if (sentinels_received == ctx->num_workers && pending_head == NULL)
            {
                break;
            }
            continue;
        }

        // Process data chunk according to sequence order.
        if (chunk->seq_id == expected_seq_id)
        {
            // Commit payload directly if chunk arrived in expected sequential order.
            write_chunk_payload(ctx->fd_out, chunk);
            expected_seq_id++;

            // Destroy committed chunk and enforce Inv4.
            chunk_destroy(&chunk);
            assert_single_ownership((const chunk_t *const *)&chunk);

            // Flush subsequent contiguous chunks already buffered in pending list.
            drain_contiguous_pending(ctx->fd_out, &pending_head, &expected_seq_id);
        }
        else
        {
            // Buffer out-of-order chunk into sorted pending list until missing predecessor arrives.
            insert_pending_chunk(&pending_head, chunk);
            chunk = NULL;
        }

        // Check exit condition if all sentinels arrived and pending chunks were flushed.
        if (sentinels_received == ctx->num_workers && pending_head == NULL)
        {
            break;
        }
    }

    // Clean up any residual pending entries if loop terminated prematurely.
    clear_pending_list(&pending_head);

    // Clean exit of writer thread.
    return NULL;
}
