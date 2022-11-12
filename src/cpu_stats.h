#ifndef CPU_STATS_H
#define CPU_STATS_H

#define NUM_STATS 1

#define CACHE_MISS_STAT 0

typedef struct cpu_stats
{
    int fd[NUM_STATS];
    long long cache_miss;
} cpu_stats_t;

int cpu_stats_open(cpu_stats_t *stats);
int cpu_stats_reset(cpu_stats_t *stats);
int cpu_stats_start(cpu_stats_t *stats);
int cpu_stats_end(cpu_stats_t *stats);
int cpu_stats_close(cpu_stats_t *stats);
#endif