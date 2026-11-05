# Technical Documentation

## 1. Overview
The pipeline architecture coordinates multi-threaded Reader, Worker, and Writer stages using bounded buffers with backpressure (`bounded_queue_t`) and atomic chunk exchange units (`chunk_t`).

---

## 2. Common Pipeline Primitive (`chunk_t`)

Defined in `include/common.h`.

### Data Structure

```c
typedef struct chunk
{
    uint8_t *data;       /**< Heap-allocated buffer for chunk payload */
    size_t capacity;     /**< Maximum capacity of buffer in bytes */
    size_t size;         /**< Current valid payload size in bytes (handles short I/O) */
    uint64_t seq_id;     /**< Monotonic sequence identifier for total pipeline ordering */
    bool is_eof;         /**< Sentinel flag: true marks EOF / pipeline teardown */
} chunk_t;
```

### Invariants

- **Inv4 (Single-Ownership Invariant)**:
  At any instant in time, exactly one stage (Reader thread, bounded queue slot, Worker thread, or Writer thread) owns a given `chunk_t*`.
  Ownership transfer (e.g. `bounded_queue_push`) or destruction (`chunk_destroy`) accepts a double pointer (`chunk_t **chunk_ref`) and zeroes the caller's reference (`*chunk_ref = NULL`), preventing dangling references and double frees.

- **Short I/O Payload Bounds**:
  For non-EOF chunks: `data != NULL`, `capacity > 0`, and `0 <= size <= capacity`.
  Accommodates short reads and partial writes from POSIX `read(2)` / `write(2)` without reallocating buffers.

- **FR-1 & FR-5 (EOF Sentinel Propagation)**:
  An EOF sentinel chunk is created with `is_eof = true`, `data = NULL`, `capacity = 0`, and `size = 0`.
  The Reader produces the sentinel at the end of the input stream; Workers forward it to downstream queues, and the Writer consumes it to cleanly terminate the pipeline.

### State Validation Assertions

- `assert_chunk_valid(const chunk_t *chunk)`:
  Validates that `chunk != NULL`. Asserts sentinel constraints (`data == NULL && size == 0`) when `is_eof` is true, and buffer bounds (`data != NULL && capacity > 0 && size <= capacity`) when `is_eof` is false.
- `assert_single_ownership(const chunk_t *const *chunk_ref)`:
  Asserts that `chunk_ref != NULL && *chunk_ref == NULL`, verifying that the caller successfully transferred or surrendered ownership.

### Lifecycle APIs

- `chunk_t *chunk_create(size_t capacity, uint64_t seq_id);`
  Allocates a `chunk_t` instance and its `data` buffer of size `capacity`. Initializes `size = 0`, `is_eof = false`, and `seq_id`.
- `chunk_t *chunk_create_sentinel(uint64_t seq_id);`
  Allocates an EOF sentinel `chunk_t` with `data = NULL`, `capacity = 0`, `size = 0`, and `is_eof = true`.
- `void chunk_destroy(chunk_t **chunk_ref);`
  Frees `(*chunk_ref)->data` and `*chunk_ref`, asserting invariants and setting `*chunk_ref = NULL` to enforce Inv4.

---

## 3. Bounded Queue (`bounded_queue_t`)

Defined in `include/bounded_queue.h`.

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

### Invariants
- `assert(q->count <= q->capacity);` holds across initialization, enqueue, and dequeue operations.

---

## 4. Worker Stage (`worker.c`)

Implemented in `src/worker.c`.

### Data Structure

```c
typedef struct worker_ctx
{
    bounded_queue_t *in_queue;
    bounded_queue_t *out_queue;
    void (*transform)(chunk_t *chunk);
} worker_ctx_t;
```

### Module APIs

- `void *worker_thread(void *arg);`
  - Main thread loop consuming chunks from `in_queue` and forwarding to `out_queue`.
  - Argument `arg` must point to an initialized `worker_ctx_t`.
  - On EOF sentinel detection (`chunk->is_eof == true`):
    - Forwards sentinel chunk to `out_queue` (FR-1, FR-5).
    - Asserts single ownership (`assert_single_ownership`).
    - Exits thread loop cleanly.
  - On standard data chunk:
    - Validates invariants using `assert_chunk_valid`.
    - Applies transformation (`transform` callback if provided, otherwise default validation).
    - Re-validates chunk state post-transformation.
    - Pushes chunk to `out_queue` and zeroes local pointer (Inv4).
- `void *worker_routine(void *arg);`
  - Alias for `worker_thread` conforming to standard thread routine naming.

### Invariants Preserved
- `Inv4`: At all points, worker holds exclusive ownership of popped chunk until pushed to `out_queue` where `chunk == NULL`.
- `FR-1, FR-5`: Sentinels demarcate EOF and propagate to downstream consumers.
