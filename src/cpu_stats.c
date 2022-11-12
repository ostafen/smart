#include "cpu_stats.h"

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <unistd.h>
#include <string.h>

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
}

void perf_event_init(struct perf_event_attr *pe)
{
    memset(pe, 0, sizeof(struct perf_event_attr));
    pe->type = PERF_TYPE_HARDWARE;
    pe->size = sizeof(struct perf_event_attr);
    pe->config = PERF_COUNT_HW_INSTRUCTIONS;
    pe->disabled = 1;
    pe->exclude_kernel = 1;
    pe->exclude_hv = 1;
}

int open_cache_miss_count()
{
    struct perf_event_attr pe;
    perf_event_init(&pe);

    return perf_event_open(&pe, 0, -1, -1, 0);
}

int cpu_stats_open(cpu_stats_t *stats)
{
    memset(stats->fd, 0, sizeof(int) * NUM_STATS);

    stats->fd[CACHE_MISS_STAT] = open_cache_miss_count();

    for (int i = 0; i < NUM_STATS; i++)
    {
        if (stats->fd[i] < 0)
            return -1;
    }
    return 0;
}

int cpu_stats_close(cpu_stats_t *stats)
{
    for (int i = 0; i < NUM_STATS; i++)
        close(stats->fd[i]);
    return 0;
}

int cpu_stats_reset(cpu_stats_t *stats)
{
    stats->cache_miss = 0;
}

int cpu_stats_start(cpu_stats_t *stats)
{
    for (int i = 0; i < NUM_STATS; i++)
    {
        if (ioctl(stats->fd[i], PERF_EVENT_IOC_RESET, 0) < 0)
            return -1;

        if (ioctl(stats->fd[i], PERF_EVENT_IOC_ENABLE, 0) < 0)
            return -1;
    }
    return 0;
}

int cpu_stats_end(cpu_stats_t *stats)
{
    for (int i = 0; i < NUM_STATS; i++)
    {
        if (ioctl(stats->fd[i], PERF_EVENT_IOC_DISABLE, 0) < 0)
            return -1;
    }

    return read(stats->fd[CACHE_MISS_STAT], &stats->cache_miss, sizeof(long long));
}