#include "cpu_stats.h"

int zero_cpu_stats(cpu_stats_t *stats)
{
    stats->cache_references    = 0;
    stats->cache_misses        = 0;
    stats->l1_cache_access     = 0;
    stats->l1_cache_misses     = 0;
    stats->branch_instructions = 0;
    stats->branch_misses       = 0;

    return 1;
}

int cpu_stats_add(cpu_stats_t *to, const cpu_stats_t *add)
{
    to->cache_references    += add->cache_references;
    to->cache_misses        += add->cache_misses;
    to->l1_cache_access     += add->l1_cache_access;
    to->l1_cache_misses     += add->l1_cache_misses;
    to->branch_instructions += add->branch_instructions;
    to->branch_misses       += add->branch_misses;
}

int cpu_stats_div(cpu_stats_t *to, const int divide_by)
{
    to->cache_references    /= divide_by;
    to->cache_misses        /= divide_by;
    to->l1_cache_access     /= divide_by;
    to->l1_cache_misses     /= divide_by;
    to->branch_instructions /= divide_by;
    to->branch_misses       /= divide_by;
}

#if defined __linux__

#define CALLING_PROCESS 0
#define ANY_CPU (-1)
#define PERF_GROUP_LEADER (-1)

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <unistd.h>
#include <string.h>

static const __u32 L1_CACHE_CONFIG = PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8);

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void perf_event_init(struct perf_event_attr *pe, __u32 type, __u64 config)
{
    memset(pe, 0, sizeof(struct perf_event_attr));
    pe->type = type;
    pe->size = sizeof(struct perf_event_attr);
    pe->config = config;
    pe->disabled = 1;
    pe->exclude_kernel = 1;
    pe->exclude_hv = 1;
}

int open_measurement(__u32 type, __u64 config, int group_fd)
{
    struct perf_event_attr pe;
    perf_event_init(&pe, type, config);

    return perf_event_open(&pe, CALLING_PROCESS, ANY_CPU, group_fd, 0);
}

int cpu_perf_open(cpu_perf_events_t *perf_events, const int cpu_stats_to_get)
{
    for (int i = 0; i < NUM_CPU_STATS; i++) perf_events->fd[i] = -1;
    perf_events->cpu_stats = cpu_stats_to_get;

    int group_leader = PERF_GROUP_LEADER;

    if (cpu_stats_to_get & CPU_STAT_LL_CACHE) {
        perf_events->fd[CPU_STATS_CACHE_REFERENCES] =
                open_measurement(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, group_leader);
        group_leader = perf_events->fd[CPU_STATS_CACHE_REFERENCES];
        perf_events->fd[CPU_STATS_CACHE_MISSES] =
                open_measurement(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, group_leader);
    }

    if (cpu_stats_to_get & CPU_STAT_L1_CACHE) {
        perf_events->fd[CPU_STATS_L1_CACHE_ACCESS] =
                open_measurement(PERF_TYPE_HW_CACHE, L1_CACHE_CONFIG | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16), group_leader);
        if (group_leader == PERF_GROUP_LEADER) group_leader = perf_events->fd[CPU_STATS_L1_CACHE_ACCESS];
        perf_events->fd[CPU_STATS_L1_CACHE_MISSES] =
                open_measurement(PERF_TYPE_HW_CACHE, L1_CACHE_CONFIG | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16), group_leader);
    }

    if (cpu_stats_to_get & CPU_STAT_BRANCHES) {
        perf_events->fd[CPU_STATS_BRANCH_INSTRUCTIONS] =
                open_measurement(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, group_leader);
        if (group_leader == PERF_GROUP_LEADER) group_leader = perf_events->fd[CPU_STATS_BRANCH_INSTRUCTIONS];
        perf_events->fd[CPU_STATS_BRANCH_MISSES] =
                open_measurement(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, group_leader);
    }

    //TODO: deal with failure to open perf events.

    return 1;
}

int cpu_perf_close(cpu_perf_events_t *perf_events)
{
    for (int i = 0; i < NUM_CPU_STATS; i++)
        if (perf_events->fd[i] >= 0) close(perf_events->fd[i]);

    return 1;
}

int cpu_perf_start(cpu_perf_events_t *perf_events)
{
    for (int i = 0; i < NUM_CPU_STATS; i++)
    {
        if (perf_events->fd[i] >= 0 && ioctl(perf_events->fd[i], PERF_EVENT_IOC_RESET, 0) < 0)
            return 0;

        if (perf_events->fd[i] >= 0 && perf_events->fd[i] && ioctl(perf_events->fd[i], PERF_EVENT_IOC_ENABLE, 0) < 0)
            return 0;
    }

    return 1;
}

int cpu_perf_end(cpu_perf_events_t *perf_events, cpu_stats_t *stats)
{
    for (int i = 0; i < NUM_CPU_STATS; i++)
    {
        if (perf_events->fd[i] >= 0 && ioctl(perf_events->fd[i], PERF_EVENT_IOC_DISABLE, 0) < 0)
            return -1;
    }

    int result = 0;
    if (perf_events->cpu_stats & CPU_STAT_L1_CACHE) {
        result |= read(perf_events->fd[CPU_STATS_L1_CACHE_ACCESS], &(stats->l1_cache_access), sizeof(long long));
        result |= read(perf_events->fd[CPU_STATS_L1_CACHE_MISSES], &(stats->l1_cache_misses), sizeof(long long));
    }

    if (perf_events->cpu_stats & CPU_STAT_LL_CACHE) {
        result |= read(perf_events->fd[CPU_STATS_CACHE_REFERENCES], &(stats->cache_references), sizeof(long long));
        result |= read(perf_events->fd[CPU_STATS_CACHE_MISSES], &(stats->cache_misses), sizeof(long long));
    }

    if (perf_events->cpu_stats & CPU_STAT_BRANCHES) {
        result |= read(perf_events->fd[CPU_STATS_BRANCH_INSTRUCTIONS], &(stats->branch_instructions), sizeof(long long));
        result |= read(perf_events->fd[CPU_STATS_BRANCH_MISSES], &(stats->branch_misses), sizeof(long long));
    }

    return result; //TODO: examine this logic, what is return value telling us?  that at least one perf counter was read?
}

#else // perf events only supported on linux - return False (0) for these calls if not supported.

int cpu_stats_open(cpu_stats_t *stats)
{
    return 0;
}

int cpu_stats_close(cpu_stats_t *stats)
{
    return 0;
}

int cpu_stats_start(cpu_stats_t *stats)
{
    return 0;
}

int cpu_stats_end(cpu_stats_t *stats)
{
    return 0;
}


int cpu_perf_open(cpu_perf_events_t *perf_events, const int cpu_stats_to_get)
{
    return 0;
}

int cpu_perf_close(cpu_perf_events_t *perf_events)
{
    return 0;
}

int cpu_perf_start(cpu_perf_events_t *perf_events)
{
    return 0;
}

int cpu_perf_end(cpu_perf_events_t *perf_events, cpu_stats_t *stats)
{
    return 0;
}
#endif
