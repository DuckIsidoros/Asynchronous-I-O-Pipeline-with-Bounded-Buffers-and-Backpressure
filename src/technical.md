# Technical Documentation: Bounded Queue

## 1. Overview
The `bounded_queue` module implements a thread-safe, FIFO bounded buffer with backpressure for pipeline blocks (`pipeline_block_t`). It uses POSIX mutexes and condition variables (`pthread_mutex_t`, `pthread_cond_t`) to coordinate producers and consumers.

## 2. Interface Specification

### Data Structures

```c
typedef struct
{
    uint64_t sequence_id;
    size_t payload_len;
    uint8_t *data;
} pipeline_block_t;

typedef struct
{
    pipeline_block_t **slots;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    bool is_shutdown;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} bounded_queue_t;
```

### Module APIs

- `int bounded_queue_init(bounded_queue_t *q, size_t capacity);`
  - Allocates queue buffer slots and initializes synchronization primitives.
  - Returns `0` on success, `-1` on invalid arguments (`!q` or `capacity == 0`) or allocation failure.

- `bool bounded_queue_push(bounded_queue_t *q, pipeline_block_t **block_ref);`
  - Thread-safe blocking push with backpressure (waits while `q->count == q->capacity && !q->is_shutdown`).
  - **Ownership Transfer**: On successful push, moves `*block_ref` into the queue buffer and sets `*block_ref = NULL` to prevent dangling references in caller. If push is rejected (e.g. shutdown or null inputs), `*block_ref` is preserved.
  - Returns `true` if item was enqueued, `false` if rejected or during shutdown.

- `bool bounded_queue_pop(bounded_queue_t *q, pipeline_block_t **out_block);`
  - Thread-safe blocking pop (waits while `q->count == 0 && !q->is_shutdown`).
  - Drains remaining items on shutdown before returning `false`.
  - Sets `*out_block = NULL` when queue is empty and shut down.
  - Returns `true` if item was dequeued, `false` otherwise.

- `void bounded_queue_shutdown(bounded_queue_t *q);`
  - Sets `is_shutdown = true` and broadcasts to `not_full` and `not_empty` conditions.
  - Safe against NULL `q`.

- `void bounded_queue_destroy(bounded_queue_t *q);`
  - Frees slots buffer and destroys mutex and condition variables.
  - Safe against NULL `q`.

## 3. Invariants
- `assert(q->count <= q->capacity);` holds across initialization, enqueue, and dequeue operations.
