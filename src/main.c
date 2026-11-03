#include "bounded_queue.h"
#include <stdio.h>
#include <assert.h>

int main(void)
{
    bounded_queue_t q;
    bounded_queue_init(&q, 4);

    pipeline_block_t b1 = {.sequence_id = 42, .payload_len = 0, .data = NULL};
    pipeline_block_t *ref = &b1;

    bounded_queue_push(&q, &ref);
    printf("Pushed block. ref is now: %p\n", (void *)ref);

    pipeline_block_t *out = NULL;
    bounded_queue_pop(&q, &out);
    printf("Popped block with sequence_id: %llu\n", (unsigned long long)out->sequence_id);

    bounded_queue_destroy(&q);
    return 0;
}