#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef struct chunk
{
    uint8_t *data;
    size_t capacity;
    size_t size;
    uint64_t seq_id;
    bool is_eof;
} chunk_t;

static inline void assert_chunk_valid(const chunk_t *chunk)
{
    assert(chunk != NULL);
    if (chunk->is_eof)
    {
        /* FR-1 & FR-5: EOF Sentinel holds no payload */
        assert(chunk->data == NULL);
        assert(chunk->size == 0);
    }
    else
    {
        /* Regular chunk: valid buffer with valid payload bounds */
        assert(chunk->capacity > 0);
        assert(chunk->data != NULL);
        assert(chunk->size <= chunk->capacity);
    }
}
static inline void assert_single_ownership(const chunk_t *const *chunk_ref)
{
    assert(chunk_ref != NULL);
    assert(*chunk_ref == NULL);
}
chunk_t *chunk_create(size_t capacity, uint64_t seq_id);
chunk_t *chunk_create_sentinel(uint64_t seq_id);
void chunk_destroy(chunk_t **chunk_ref);

#endif