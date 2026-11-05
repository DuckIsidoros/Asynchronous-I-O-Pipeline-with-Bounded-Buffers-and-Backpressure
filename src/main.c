#include "common.h"
#include "bounded_queue.h"
#include "worker.h"
#include "reader.h"
#include "writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

// Print application CLI usage parameters.
static void print_usage(const char *prog_name)
{
    // Output formatted help instructions to stderr.
    fprintf(stderr, "Usage: %s [-i input_file] [-o output_file] [-w workers] [-q queue_cap] [-b chunk_size]\n", prog_name);
    fprintf(stderr, "  -i <path>   Input file path (defaults to stdin)\n");
    fprintf(stderr, "  -o <path>   Output file path (defaults to stdout)\n");
    fprintf(stderr, "  -w <num>    Number of worker threads (default: 4)\n");
    fprintf(stderr, "  -q <cap>    Bounded queue capacity (default: 16)\n");
    fprintf(stderr, "  -b <size>   Chunk buffer size in bytes (default: 65536)\n");
}

// Pipeline supervisor entry point managing stage lifecycles and thread barriers.
int main(int argc, char *argv[])
{
    // Default pipeline configuration values.
    const char *input_path = NULL;
    const char *output_path = NULL;
    size_t num_workers = 4;
    size_t queue_cap = 16;
    size_t chunk_size = 65536;

    // Parse command line configuration options.
    int opt;
    while ((opt = getopt(argc, argv, "i:o:w:q:b:h")) != -1)
    {
        switch (opt)
        {
        case 'i':
            input_path = optarg;
            break;
        case 'o':
            output_path = optarg;
            break;
        case 'w':
            num_workers = (size_t)strtoul(optarg, NULL, 10);
            break;
        case 'q':
            queue_cap = (size_t)strtoul(optarg, NULL, 10);
            break;
        case 'b':
            chunk_size = (size_t)strtoul(optarg, NULL, 10);
            break;
        case 'h':
        default:
            print_usage(argv[0]);
            return (opt == 'h') ? 0 : 1;
        }
    }

    // Validate configuration bounds.
    if (num_workers == 0 || queue_cap == 0 || chunk_size == 0)
    {
        fprintf(stderr, "Error: workers, queue_cap, and chunk_size must all be positive.\n");
        return 1;
    }

    // Open input file descriptor or default to stdin.
    int fd_in = STDIN_FILENO;
    if (input_path != NULL && strcmp(input_path, "-") != 0)
    {
        fd_in = open(input_path, O_RDONLY);
        if (fd_in < 0)
        {
            perror("Failed to open input file");
            return 1;
        }
    }

    // Open output file descriptor or default to stdout.
    int fd_out = STDOUT_FILENO;
    if (output_path != NULL && strcmp(output_path, "-") != 0)
    {
        fd_out = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0)
        {
            perror("Failed to open output file");
            if (fd_in != STDIN_FILENO)
            {
                close(fd_in);
            }
            return 1;
        }
    }

    // Initialize pipeline stage queues: q1 (Reader->Workers) and q2 (Workers->Writer).
    bounded_queue_t q1;
    bounded_queue_t q2;
    if (bounded_queue_init(&q1, queue_cap) != 0 || bounded_queue_init(&q2, queue_cap) != 0)
    {
        fprintf(stderr, "Failed to initialize bounded queues\n");
        return 1;
    }

    // Allocate thread tracking arrays for worker pool.
    pthread_t *worker_tids = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);
    worker_ctx_t *worker_contexts = (worker_ctx_t *)malloc(sizeof(worker_ctx_t) * num_workers);
    assert(worker_tids != NULL && worker_contexts != NULL);

    // Initialize reader context and spawn reader thread.
    reader_ctx_t r_ctx = {
        .fd_in = fd_in,
        .chunk_size = chunk_size,
        .out_queue = &q1,
        .num_workers = num_workers
    };
    pthread_t reader_tid;
    pthread_create(&reader_tid, NULL, reader_thread, &r_ctx);

    // Spawn multi-threaded worker pool.
    for (size_t i = 0; i < num_workers; ++i)
    {
        worker_contexts[i].in_queue = &q1;
        worker_contexts[i].out_queue = &q2;
        worker_contexts[i].transform = NULL;
        pthread_create(&worker_tids[i], NULL, worker_thread, &worker_contexts[i]);
    }

    // Initialize writer context and spawn writer thread.
    writer_ctx_t w_ctx = {
        .fd_out = fd_out,
        .in_queue = &q2,
        .num_workers = num_workers
    };
    pthread_t writer_tid;
    pthread_create(&writer_tid, NULL, writer_thread, &w_ctx);

    // Join reader thread upon completion.
    pthread_join(reader_tid, NULL);

    // Join all worker pool threads upon sentinel-driven exit.
    for (size_t i = 0; i < num_workers; ++i)
    {
        pthread_join(worker_tids[i], NULL);
    }

    // Join writer thread upon completion of ordered egress flush.
    pthread_join(writer_tid, NULL);

    // Close open file descriptors if opened from filesystem paths.
    if (fd_in != STDIN_FILENO)
    {
        close(fd_in);
    }
    if (fd_out != STDOUT_FILENO)
    {
        close(fd_out);
    }

    // Destroy bounded synchronization queues.
    bounded_queue_destroy(&q1);
    bounded_queue_destroy(&q2);

    // Release allocated worker metadata buffers.
    free(worker_tids);
    free(worker_contexts);

    // Return success status code.
    return 0;
}
