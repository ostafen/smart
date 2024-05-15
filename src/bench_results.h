#ifndef SMART_BENCH_RESULTS_H
#define SMART_BENCH_RESULTS_H

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "cpu_stats.h"


/*
 * The various results of attempting to measure benchmark times for an algorithm.
 */
enum measurement_status
{
    SUCCESS,
    TIMED_OUT,
    CANNOT_SEARCH,
    ERROR
};

/*
 * The specific measurements taken for each invocation of an algorithm search.
 */
typedef struct algo_measurements
{
    double *search_times;
    double *pre_times;
    cpu_stats_t *cpu_stats;
} algo_measurements_t;

/*
 * Various statistics which can be computed from the measurements of an algorithm for a single pattern length.
 */
typedef struct algo_statistics
{
    double min_search_time;
    double max_search_time;
    double mean_search_time;
    double median_search_time;
    double std_search_time;       // standard deviation of search times expressed in milliseconds.
    double std_search_time_gbs;   // standard deviation of search times expressed in gigabytes per second.

    double min_pre_time;
    double max_pre_time;
    double mean_pre_time;
    double median_pre_time;

    double min_total_time;
    double max_total_time;
    double mean_total_time;
    double median_total_time;
    double std_total_time;

    cpu_stats_t sum_cpu_stats;
} algo_statistics_t;

/*
 * The result of benchmarking a single algorithm, including all measurements and statistics derived from them,
 * for a single length of pattern.
 */
typedef struct algo_results
{
    int algo_id;
    enum measurement_status success_state;
    algo_measurements_t measurements;
    algo_statistics_t statistics;
    int occurrence_count;
} algo_results_t;

/*
 * All measurements and statistics from benchmarking all selected algorithms for a set size of pattern.
 */
typedef struct benchmark_results
{
    int pattern_length;
    algo_results_t *algo_results; // dynamically allocated array of results for the number of algorithms.
} benchmark_results_t;


/*
 * Computes the minimum and maximum values contained in the list of doubles of length n,
 * and puts them into min_value and max_value. If the list is empty, the min and max will be zero.
 */
void compute_min_max(const double *T, int n, double *min_value, double *max_value)
{
    double min = 0.0;
    double max = 0.0;
    if (n > 0)
    {
        min = max = T[0];
        for (int i = 1; i < n; i++)
        {
            if (min > T[i]) min = T[i];
            if (max < T[i]) max = T[i];
        }
    }
    *min_value = min;
    *max_value = max;
}

/*
 * Computes and returns the mean average of a list of search times of size n.
 */
double compute_average(const double *T, int n)
{
    double avg = 0.0;
    for (int i = 0; i < n; i++)
        avg += T[i];

    return avg / n;
}

/*
 * Computes and returns the sum of a list of CPU measurements of size n
 */
cpu_stats_t compute_sum_cpu_stats(const cpu_stats_t *stats, int n)
{
    cpu_stats_t mean;
    zero_cpu_stats(&mean);
    for (int i = 0; i < n; i++)
        cpu_stats_add(&mean, stats + i);

    return mean;
}

/*
 * Computes and returns the median of an array T of doubles of size n.
 * It creates a copy of the array and sorts it before obtaining the median.
 */
double compute_median(const double *T, int n)
{
    // Sort the array passed in:
    double sorted[n];
    memcpy(sorted, T, sizeof(double) * n);
    qsort(sorted, n, sizeof(double), double_compare);

    // if the list of doubles  has an even number of elements:

    if (n % 2 == 0)
    {
        return (sorted[n / 2 - 1] + sorted[n / 2]) / 2; // return mean of n/2-1 and n/2 elements.
    }
    else
    {
        return sorted[(n + 1) / 2 - 1]; // return the element in the middle of the sorted array.
    }
}

/*
 * Computes and returns the sample standard deviation given a mean average, and a list of search times of size n.
 *
 * Sample standard deviation differs from population standard deviation, by dividing by one less than the number of
 * samples.  This biases the sample standard deviation to be slightly bigger, to account for likely wider variation
 * in the population as a whole.
 *
 * See: https://www.statology.org/population-vs-sample-standard-deviation/
 */
double compute_std(double avg, double *T, int n)
{
    double std = 0.0;
    for (int i = 0; i < n; i++)
        std += pow(avg - T[i], 2.0);
    int sample_divisor = MAX(1, n - 1);
    return sqrt(std / sample_divisor);
}

/*
 * Calculates statistics for an algorithm for a given pattern length, given the measurements taken for that
 * algorithm over a number of runs.
 */
void calculate_algo_statistics(algo_results_t *results, int num_measurements, int text_length)
{
    // Compute total of pre and search times:
    double *total_times = malloc(sizeof(double) * num_measurements);
    for (int i = 0; i < num_measurements; i++)
    {
        total_times[i] = results->measurements.pre_times[i] + results->measurements.search_times[i];
    }

    // Compute min and max pre, search and total times:
    compute_min_max(results->measurements.pre_times, num_measurements,
                    &(results->statistics.min_pre_time), &(results->statistics.max_pre_time));
    compute_min_max(results->measurements.search_times, num_measurements,
                    &(results->statistics.min_search_time), &(results->statistics.max_search_time));
    compute_min_max(total_times, num_measurements,
                    &(results->statistics.min_total_time), &(results->statistics.max_total_time));

    // Compute mean pre, search and total times and the standard deviation:
    results->statistics.mean_pre_time = compute_average(results->measurements.pre_times, num_measurements);
    results->statistics.mean_search_time = compute_average(results->measurements.search_times, num_measurements);
    results->statistics.std_search_time = compute_std(results->statistics.mean_search_time,
                                                      results->measurements.search_times, num_measurements);
    results->statistics.mean_total_time = compute_average(total_times, num_measurements);
    results->statistics.std_total_time = compute_std(results->statistics.mean_total_time, total_times, num_measurements);

    // Compute standard deviation for Gigabytes per second:
    double gbs[num_measurements];
    for (int i = 0; i < num_measurements; i++)
        gbs[i] = GBs(results->measurements.search_times[i], text_length);
    double mean_gbs = compute_average(gbs, num_measurements);
    results->statistics.std_search_time_gbs = compute_std(mean_gbs, gbs, num_measurements);

    // Compute median pre, search and total times:
    results->statistics.median_pre_time = compute_median(results->measurements.pre_times, num_measurements);
    results->statistics.median_search_time = compute_median(results->measurements.search_times, num_measurements);
    results->statistics.median_total_time = compute_median(total_times, num_measurements);

    // Compute sum cpu stats:
    results->statistics.sum_cpu_stats = compute_sum_cpu_stats(results->measurements.cpu_stats, num_measurements);

    free(total_times);
}

/*
 * Allocates memory for all benchmark results given the number of different patern lengths, the number of algorithms,
 * and the number of runs per pattern length for each algorithm.
 */
void allocate_benchmark_results(benchmark_results_t bench_result[], int num_pattern_lengths, int num_algos, int num_runs)
{
    // For each pattern length we process, allocate space for bench_result for all the algorithms:
    for (int i = 0; i < num_pattern_lengths; i++)
    {
        bench_result[i].algo_results = (algo_results_t *)malloc(sizeof(algo_results_t) * num_algos);

        // For each algorithm we process for a pattern length, allocate space for all the measurements it will make:
        for (int j = 0; j < num_algos; j++)
        {
            bench_result[i].algo_results[j].measurements.search_times = (double *)malloc(sizeof(double) * num_runs);
            bench_result[i].algo_results[j].measurements.pre_times = (double *)malloc(sizeof(double) * num_runs);
            bench_result[i].algo_results[j].measurements.cpu_stats = (cpu_stats_t *)malloc(sizeof(cpu_stats_t) * num_runs);
        }
    }
}

/*
 * Frees memory allocated for benchmark results.
 */
void free_benchmark_results(benchmark_results_t bench_result[], int num_pattern_lengths, int num_algos)
{
    // For each pattern length we process, allocate space for bench_result for all the algorithms:
    for (int i = 0; i < num_pattern_lengths; i++)
    {
        // For each algorithm we process for a pattern length, allocate space for all the measurements it will make:
        for (int j = 0; j < num_algos; j++)
        {
            free(bench_result[i].algo_results[j].measurements.search_times);
            free(bench_result[i].algo_results[j].measurements.pre_times);
            free(bench_result[i].algo_results[j].measurements.cpu_stats);
        }
        free(bench_result[i].algo_results);
    }
}


#endif //SMART_BENCH_RESULTS_H
