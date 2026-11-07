#include "common.h"
#include <stdlib.h>
#include <assert.h>

// Allocate and initialize a data chunk with pre-allocated payload buffer.
chunk_t *chunk_create(size_t capacity, uint64_t seq_id)
{
    // Allocate chunk control structure.
    chunk_t *chunk = (chunk_t *)malloc(sizeof(chunk_t));
    if (!chunk)
    {
        return NULL;
    }

    // Allocate chunk data payload buffer if capacity requested.
    if (capacity > 0)
    {
        chunk->data = (uint8_t *)malloc(capacity);
        if (!chunk->data)
        {
            free(chunk);
            return NULL;
        }
    }
    else
    {
        chunk->data = NULL;
    }

    // Initialize chunk fields.
    chunk->capacity = capacity;
    chunk->size = 0;
    chunk->seq_id = seq_id;
    chunk->is_eof = false;

    // Validate chunk state invariants before returning.
    assert_chunk_valid(chunk);
    return chunk;
}

// Allocate and initialize an EOF sentinel chunk carrying zero payload.
chunk_t *chunk_create_sentinel(uint64_t seq_id)
{
    // Allocate sentinel chunk control structure.
    chunk_t *chunk = (chunk_t *)malloc(sizeof(chunk_t));
    if (!chunk)
    {
        return NULL;
    }

    // Initialize sentinel chunk fields according to FR-1 and FR-5 invariants.
    chunk->data = NULL;
    chunk->capacity = 0;
    chunk->size = 0;
    chunk->seq_id = seq_id;
    chunk->is_eof = true;

    // Validate sentinel chunk invariants.
    assert_chunk_valid(chunk);
    return chunk;
}

// Destroy chunk, free payload buffer, and enforce Inv4 single ownership transfer.
void chunk_destroy(chunk_t **chunk_ref)
{
    // Check for null pointer reference before destruction.
    if (!chunk_ref || !*chunk_ref)
    {
        return;
    }

    // Dereference chunk and validate state invariants.
    chunk_t *chunk = *chunk_ref;
    assert_chunk_valid(chunk);

    // Free data buffer if present.
    if (chunk->data)
    {
        free(chunk->data);
        chunk->data = NULL;
    }

    // Free control structure and zero out caller reference to prevent dangling pointers.
    free(chunk);
    *chunk_ref = NULL;

    // Enforce Inv4 single-ownership invariant assertion.
    assert_single_ownership((const chunk_t *const *)chunk_ref);
}
