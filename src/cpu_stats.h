#ifndef CPU_STATS_H
#define CPU_STATS_H

/*
 * Definitions of stats we can capture.
 */
#define NUM_CPU_STATS                    6
#define CPU_STATS_CACHE_REFERENCES       0
#define CPU_STATS_CACHE_MISSES           1
#define CPU_STATS_L1_CACHE_ACCESS        2
#define CPU_STATS_L1_CACHE_MISSES        3
#define CPU_STATS_BRANCH_INSTRUCTIONS    4
#define CPU_STATS_BRANCH_MISSES          5

/*
 * Bitmask to capture Level 1 Data CPU Cache
 */
static const int CPU_STAT_L1_CACHE = 0x01;

/*
 * Bitmask to capture Last Level Data CPU Cache
 */
static const int CPU_STAT_LL_CACHE = 0x02;

/*
 * Bitmask to capture branch CPU stats.
 */
static const int CPU_STAT_BRANCHES = 0x04;

/*
 * Holds the file descriptors for CPU performance events.
 */
typedef struct cpu_perf_events
{
    int fd[NUM_CPU_STATS];
    int cpu_stats;
} cpu_perf_events_t;

/*
 * Holds the CPU stats captured.
 */
typedef struct cpu_stats
{
    long long cache_references;
    long long cache_misses;
    long long l1_cache_access;
    long long l1_cache_misses;
    long long branch_instructions;
    long long branch_misses;
} cpu_stats_t;

/*
 * Opens the file descriptors for all the cpu stats we want to capture.
 * cpu_stats_to_get is a bitmask which defines which cpu stats to get:
 * CPU_STAT_L1_CACHE, CPU_STAT_LL_CACHE or CPU_STAT_BRANCHES.
 */
int cpu_perf_open(cpu_perf_events_t *perf_events, int cpu_stats_to_get);

/*
 * Starts capturing the cpu stats we previously opened.
 */
int cpu_perf_start(cpu_perf_events_t *perf_events);

/*
 * Stops capturing the cpu stats we started capturing, and places the results in stats.
 */
int cpu_perf_end(cpu_perf_events_t *perf_events, cpu_stats_t *stats);

/*
 * Closes the file descriptors for the cpu stats we opened for capture.
 */
int cpu_perf_close(cpu_perf_events_t *perf_events);

/*
 * Sets all stats to zero.
 */
int zero_cpu_stats(cpu_stats_t *stats);

/*
 * Adds two cpu stats together, placing the result of adding to and add in to.
 */
int cpu_stats_add(cpu_stats_t *to, const cpu_stats_t *add);

/*
 * Divides the cpu stats by an integer.
 */
int cpu_stats_div(cpu_stats_t *to, const int divide_by);

#endif