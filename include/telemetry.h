#ifndef TELEMETRY_H
#define TELEMETRY_H
#include <stddef.h>
#include "bounded_queue.h"
#include <stdint.h>

typedef struct
{
    long max_rss_kb;
    uint64_t rchar;
    uint64_t wchar;
    uint64_t read_bytes;
    uint64_t write_bytes;
} process_telemetry_t;

int telemetry_collect_process(process_telemetry_t *stats);
int telemetry_export_csv(
    const char *csv_path,
    size_t workers,
    size_t chunk_size,
    const queue_telemetry_t *q1,
    const queue_telemetry_t *q2,
    const process_telemetry_t *process);
#endif
