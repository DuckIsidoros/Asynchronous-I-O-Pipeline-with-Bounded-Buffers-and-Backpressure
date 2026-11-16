
#include "telemetry.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/resource.h>

int telemetry_collect_process(process_telemetry_t *stats)
{
    if (stats == NULL)
        return -1;

    memset(stats, 0, sizeof(*stats));

    /* Collect peak RSS */
    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage) != 0)
        return -1;

    stats->max_rss_kb = usage.ru_maxrss;

    /* Collect Linux process I/O counters */
    FILE *file = fopen("/proc/self/io", "r");

    if (file == NULL)
        return -1;

    char key[64];
    uint64_t value;
    unsigned int found = 0;

    while (fscanf(file, "%63s %" SCNu64, key, &value) == 2)
    {
        if (strcmp(key, "rchar:") == 0)
        {
            stats->rchar = value;
            found |= 1u;
        }
        else if (strcmp(key, "wchar:") == 0)
        {
            stats->wchar = value;
            found |= 2u;
        }
        else if (strcmp(key, "read_bytes:") == 0)
        {
            stats->read_bytes = value;
            found |= 4u;
        }
        else if (strcmp(key, "write_bytes:") == 0)
        {
            stats->write_bytes = value;
            found |= 8u;
        }
    }

    int close_result = fclose(file);

    if (close_result != 0 || found != 15u)
        return -1;

    return 0;
}

int telemetry_export_csv(
    const char *csv_path,
    size_t workers,
    size_t chunk_size,
    const queue_telemetry_t *q1,
    const queue_telemetry_t *q2,
    const process_telemetry_t *process)
{
    if (!csv_path || !q1 || !q2 || !process)
        return -1;

    FILE *file = fopen(csv_path, "a+");
    if (!file)
        return -1;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return -1;
    }

    long file_size = ftell(file);
    if (file_size < 0)
    {
        fclose(file);
        return -1;
    }

    if (file_size == 0)
    {
        if (fprintf(file,
            "workers,queue_capacity,chunk_size,"
            "q1_max_occupancy,q2_max_occupancy,"
            "q1_producer_stall_ns,q1_consumer_stall_ns,"
            "q2_producer_stall_ns,q2_consumer_stall_ns,"
            "max_rss_kb,rchar,wchar,read_bytes,write_bytes\n") < 0)
        {
            fclose(file);
            return -1;
        }
    }

    int result = fprintf(file,
        "%zu,%zu,%zu,%zu,%zu,"
        "%llu,%llu,%llu,%llu,"
        "%ld,%llu,%llu,%llu,%llu\n",
        workers,
        q1->capacity,
        chunk_size,
        q1->max_occupancy,
        q2->max_occupancy,
        (unsigned long long)q1->producer_stall_ns,
        (unsigned long long)q1->consumer_stall_ns,
        (unsigned long long)q2->producer_stall_ns,
        (unsigned long long)q2->consumer_stall_ns,
        process->max_rss_kb,
        (unsigned long long)process->rchar,
        (unsigned long long)process->wchar,
        (unsigned long long)process->read_bytes,
        (unsigned long long)process->write_bytes
    );

    int close_result = fclose(file);

    if (result < 0 || close_result != 0)
        return -1;

    return 0;
}
