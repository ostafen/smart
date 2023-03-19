#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "timer.h"
#include "defines.h"
#include "config.h"
#include "commands.h"
#include "algorithms.h"
#include "cpu_pinning.h"
#include "data_sources.h"
#include "bench_results.h"
#include "output.h"

static const char *DOTS = "................................................................";

/*
 * Prints a percentage on a line which overwrites the previous percentage value using \b back.
 */
void print_percentage(int perc)
{
    if (perc < 10 || perc > 100)
        printf("\b\b\b\b[%d%%]", perc);
    else if (perc < 100)
        printf("\b\b\b\b\b[%d%%]", perc);
    fflush(stdout);
}

/*
 * Generates the patterns to use to search with.
 * If a pattern was supplied by the user, all the runs use the same pattern specified on the command line.
 * Otherwise, it builds a list of random patterns of size m by randomly extracting them from a text T of size n.
 */
void gen_patterns(const run_command_opts_t *opts, unsigned char *patterns[], int m, const unsigned char *T, int n, int num_patterns)
{
    if (opts->pattern != NULL)
    {
        for (int i = 0; i < num_patterns; i++)
            memcpy(patterns[i], opts->pattern, m);
    }
    else
    {
        for (int i = 0; i < num_patterns; i++)
        {
            int k = n == m ? 0 : rand() % (n - m);
            memcpy(patterns[i], T + k, m);
        }
    }
}

/*
 * Benchmarks an algorithm against a list of patterns of size m on a text T of size n, using the options provided.
 * Returns the status of attempting to measure the algorithm performance.
 */
enum measurement_status run_algo(unsigned char **pattern_list, int m,
                                 unsigned char *T, int n, const run_command_opts_t *opts,
                                 search_function *search_func, algo_results_t *results)
{
    enum measurement_status status = SUCCESS; // assume success, errors will update it.

    unsigned char P[m + 1];
    results->occurrence_count = 0;
    cpu_perf_events_t perf_events;
    int getting_cpu_stats = opts->cpu_stats ? cpu_perf_open(&perf_events, opts->cpu_stats) : 0;

    for (int k = 0; k < opts->num_runs; k++)
    {
        print_percentage((100 * (k + 1)) / opts->num_runs);

        memcpy(P, pattern_list[k], sizeof(unsigned char) * (m + 1));
        results->measurements.pre_times[k] = results->measurements.search_times[k] = 0.0;
        zero_cpu_stats(&(results->measurements.cpu_stats[k]));
        if (getting_cpu_stats) {
            cpu_perf_start(&perf_events);
        }

        int occur = search_func(P, m, T, n, &(results->measurements.search_times[k]), &(results->measurements.pre_times[k]));

        if (getting_cpu_stats) {
            cpu_perf_end(&perf_events, &(results->measurements.cpu_stats[k]));
        }

        if (occur == 0 || occur == ERROR_SEARCHING)
        {
            status = ERROR;
            break; // there must be at least one match for each text and pattern (patterns are extracted from text).
        }

        if (occur == INFO_CANNOT_SEARCH) {
            status = CANNOT_SEARCH; // negative value returned from search means it cannot search this pattern (e.g. it is too short)
            break;
        }

        results->occurrence_count += occur;

        if (results->measurements.search_times[k] > opts->time_limit_millis)
        {
            status = TIMED_OUT;
            break;
        }
    }

    if (getting_cpu_stats) cpu_perf_close(&perf_events);

    return status;
}

/*
 * Gets the results formatted according to the run options.
 */
void get_results_info(char output_line[MAX_LINE_LEN], const run_command_opts_t *opts, algo_results_t *results)
{
    char occurence[MAX_LINE_LEN];
    if (opts->occ)
    {
        snprintf(occurence, MAX_LINE_LEN, "occ(%d)", results->occurrence_count);
    }
    else
    {
        occurence[0] = STR_END_CHAR;
    }

    if (opts->cpu_stats) {
        char cpu_stats[STR_BUF];
        char cpu_stat[STR_BUF];
        cpu_stats[0] = STR_END_CHAR;
        if (results->statistics.sum_cpu_stats.l1_cache_access > 0) {
            snprintf(cpu_stat, STR_BUF, "L1:%.*f%%",
                     opts->precision, (double) results->statistics.sum_cpu_stats.l1_cache_misses /
                     results->statistics.sum_cpu_stats.l1_cache_access * 100);
            strcat(cpu_stats, cpu_stat);
        }

        if (results->statistics.sum_cpu_stats.cache_references > 0)
        {
            snprintf(cpu_stat, STR_BUF, strlen(cpu_stats) > 0 ? "  LL:%.*f%%" : "LL:%.*f%%",
                     opts->precision, (double) results->statistics.sum_cpu_stats.cache_misses /
                     results->statistics.sum_cpu_stats.cache_references * 100);
            strcat(cpu_stats, cpu_stat);
        }

        if (results->statistics.sum_cpu_stats.branch_instructions > 0) {
            snprintf(cpu_stat, STR_BUF, strlen(cpu_stats) > 0 ? "  Br:%.*f%%" : "Br:%.*f%%",
                     opts->precision, (double) results->statistics.sum_cpu_stats.branch_misses /
                     results->statistics.sum_cpu_stats.branch_instructions * 100);
            strcat(cpu_stats, cpu_stat);
        }

        if (opts->pre)
        {
            snprintf(output_line, MAX_LINE_LEN, "\t(%.*f - %.*f, %.*f, %.*f) + (%.*f - %.*f, %.*f ± %.*f, %.*f) ms\t   %s\t%s",
                     opts->precision, results->statistics.min_pre_time,
                     opts->precision, results->statistics.max_pre_time,
                     opts->precision, results->statistics.mean_pre_time,
                     opts->precision, results->statistics.median_pre_time,
                     opts->precision, results->statistics.min_search_time,
                     opts->precision, results->statistics.max_search_time,
                     opts->precision, results->statistics.mean_search_time,
                     opts->precision, results->statistics.std_search_time,
                     opts->precision, results->statistics.median_search_time,
                     cpu_stats, occurence);
        }
        else
        {
            snprintf(output_line, MAX_LINE_LEN, "\t%.*f - %.*f, %.*f ± %.*f, %.*f ms\t   %s\t%s",
                     opts->precision, results->statistics.min_total_time,
                     opts->precision, results->statistics.max_total_time,
                     opts->precision, results->statistics.mean_total_time,
                     opts->precision, results->statistics.std_total_time,
                     opts->precision, results->statistics.median_total_time,
                     cpu_stats, occurence);
        }
    }
    else
    {
        if (opts->pre)
        {
            snprintf(output_line, MAX_LINE_LEN, "\t(%.*f - %.*f, %.*f, %.*f) + (%.*f - %.*f, %.*f ± %.*f, %.*f) ms\t%s",
                     opts->precision, results->statistics.min_pre_time,
                     opts->precision, results->statistics.max_pre_time,
                     opts->precision, results->statistics.mean_pre_time,
                     opts->precision, results->statistics.median_pre_time,
                     opts->precision, results->statistics.min_search_time,
                     opts->precision, results->statistics.max_search_time,
                     opts->precision, results->statistics.mean_search_time,
                     opts->precision, results->statistics.std_search_time,
                     opts->precision, results->statistics.median_search_time,
                     occurence);
        }
        else
        {
            snprintf(output_line, MAX_LINE_LEN, "\t%.*f - %.*f, %.*f ± %.*f, %.*f ms\t%s",
                     opts->precision, results->statistics.min_total_time,
                     opts->precision, results->statistics.max_total_time,
                     opts->precision, results->statistics.mean_total_time,
                     opts->precision, results->statistics.std_total_time,
                     opts->precision, results->statistics.median_total_time,
                     occurence);
        }
    }
}

/*
 * Prints benchmark results for an algorithm run.
 */
void print_benchmark_res(const run_command_opts_t *opts, algo_results_t *results)
{
    switch (results->success_state)
    {
        case SUCCESS:
        {
            char results_line[MAX_LINE_LEN];
            get_results_info(results_line, opts, results);
            printf("\b\b\b\b\b.[OK]  %s\n", results_line);
            break;
        }
        case CANNOT_SEARCH:
        {
            printf("\b\b\b\b\b.[--]  \n");
            break;
        }
        case TIMED_OUT:
        {
            printf("\b\b\b\b\b\b.[OUT]  \n");
            break;
        }
        case ERROR:
        {
            printf("\b\b\b\b\b\b\b\b.[ERROR] \n");
            break;
        }
    }
}

/*
 * Prints the status of benchmarking for an algorithm.
 */
void print_benchmark_status(const int algo, const algo_info_t *algorithms)
{
    char fail_flag;
    char case_algo_name[ALGO_NAME_LEN];
    if (algorithms->passed_tests[algo])
    {
        fail_flag = ' ';
        set_upper_case_algo_name(case_algo_name, algorithms->algo_names[algo]);
    }
    else
    {
        fail_flag = '*';
        set_lower_case_algo_name(case_algo_name, algorithms->algo_names[algo]);
    }

    char header_line[MAX_LINE_LEN];
    snprintf(header_line, MAX_LINE_LEN, "\t - [%d/%d] %c %s ", (algo + 1), algorithms->num_algos, fail_flag, case_algo_name);

    char output_line[MAX_LINE_LEN];
    size_t header_len = strlen(header_line);
    int num_dots = header_len > BENCHMARK_HEADER_LEN ? 0 : (int)(BENCHMARK_HEADER_LEN - header_len);
    snprintf(output_line, MAX_LINE_LEN, "%s%.*s", header_line, num_dots, DOTS);

    printf("%s", output_line);
    fflush(stdout);
}

/*
 * Benchmarks all selected algorithms using a set of random patterns of a set length.
 */
int benchmark_algos_with_patterns(algo_results_t *results, const run_command_opts_t *opts, unsigned char *T, int n,
                                  unsigned char **pattern_list, int m, const algo_info_t *algorithms)
{
    info("\n------------------------------------------------------------");
    info("\tSearching for a set of %d patterns with length %d", opts->num_runs, m);

    const char *pre_header = opts->pre ? "(preprocessing) + (search): " : "";
    info("\tTesting %d algorithms                   %smin - max, mean ± stddev, median", algorithms->num_algos, pre_header);

    for (int algo = 0; algo < algorithms->num_algos; algo++)
    {
        print_benchmark_status(algo, algorithms);

        results[algo].algo_id = algo;
        results[algo].success_state = run_algo(pattern_list, m, T, n, opts, algorithms->algo_functions[algo], results + algo);

        if (results[algo].success_state == SUCCESS)
            calculate_algo_statistics(results + algo, opts->num_runs);

        print_benchmark_res(opts, results + algo);
    }
}

/*
 * Prints statistics about the text contained in T of length n.
 */
void print_text_info(const unsigned char *T, int n)
{
    info("Text buffer of dimension %d byte", n);

    int freq[SIGMA];
    compute_frequency(T, n, freq);

    int alphabet_size, max_code;
    compute_alphabet_info(freq, &alphabet_size, &max_code);

    info("Alphabet of %d characters.", alphabet_size);
    info("Greater chararacter has code %d.\n", max_code);
}

/*
 * Fills the text buffer T with opts->text_size of data.
 * If a simple search, and a text was specified, then that will be used.
 * If random text was specified, random text of the buffer size and alphabet will be used.
 * If files were specified, then all files found will be loaded up to the size of the buffer.
 * If no data can be loaded, it will error and exit.
 * Returns the size of the text loaded.
 */
int get_text(const smart_config_t *smart_config, run_command_opts_t *opts, unsigned char *T)
{
    int size = 0;
    switch (opts->data_source) {
        case DATA_SOURCE_RANDOM: {
            info("Generating random text with alphabet size of %d", opts->alphabet_size);
            size = gen_random_text(opts->alphabet_size, T, opts->text_size);
            break;
        }
        case DATA_SOURCE_FILES: {
            info("Loading search text from files specified with the %s option.", OPTION_LONG_TEXT_SOURCE);
            size = gen_search_text(smart_config, opts->data_sources, T, opts->text_size, opts->fill_buffer);
            break;
        }
        case DATA_SOURCE_USER: {
            info("Using search data supplied on the command line with the %s option.", OPTION_LONG_SEARCH_DATA);
            size = gen_user_data(opts, T);
            break;
        }
        default: {
            error_and_exit("Undefined source for data: %d\n", opts->data_source);
        }
    }

    if (size <= 0)
    {
        error_and_exit("Could not load any data to search with.\n");
    }

    return size;
}

/*
 * Returns the number of different pattern lengths to benchmark.
 * Sets the maximum pattern length which will be benchmarked in max_pattern_lengths;
 * If the user has supplied a pattern, there is only one.
 * Otherwise, the pattern lengths are defined by the pattern length info.
 */
int get_num_pattern_lengths_to_run(const run_command_opts_t *opts, int *max_pattern_length)
{
    int num_pattern_lengths;

    // If the user supplies a pattern, we just have one, otherwise get the pattern lengths to use.
    if (opts->pattern == NULL)
    {
        *max_pattern_length = get_max_pattern_length(&(opts->pattern_info), opts->text_size);
        num_pattern_lengths = get_num_pattern_lengths(&(opts->pattern_info), opts->text_size);
        if (num_pattern_lengths == 1)
        {
            info("Benchmarking with 1 pattern length of %d.", opts->pattern_info.pattern_min_len);
        }
        else
        {
            info("Benchmarking with %d pattern lengths, from %d to %d, incrementing by %c %d.", num_pattern_lengths,
                 opts->pattern_info.pattern_min_len, *max_pattern_length,
                 opts->pattern_info.increment_operator, opts->pattern_info.increment_by);
        }
    }
    else
    {
        num_pattern_lengths = 1;
        *max_pattern_length = opts->pattern_info.pattern_min_len;
        info("Benchmarking with a user supplied pattern of length %d.", opts->pattern_info.pattern_min_len);
    }

    return num_pattern_lengths;
}

/*
 * Allocates memory to hold the patterns for benchmarking.
 */
void allocate_pattern_matrix(unsigned char **M, const int num_entries, const int pattern_length)
{
    size_t element_size = sizeof(unsigned char) * (pattern_length + 1);
    for (int i = 0; i < num_entries; i++)
        M[i] = (unsigned char *)malloc(element_size);
}

/*
 * Frees memory allocated for benchmarking patterns.
 */
void free_pattern_matrix(unsigned char **M, size_t num_entries)
{
    for (int i = 0; i < num_entries; i++)
        free(M[i]);
}

/*
 * Benchmarks all algorithms over a text T for all pattern lengths.
 */
void benchmark_algorithms_with_text(const smart_config_t *smart_config, const run_command_opts_t *opts,
                                    unsigned char *T, int n, const algo_info_t *algorithms)
{
    int max_pattern_length;
    int num_pattern_lengths = get_num_pattern_lengths_to_run(opts, &max_pattern_length);

    benchmark_results_t results[num_pattern_lengths];
    unsigned char *pattern_list[opts->num_runs];

    allocate_benchmark_results(results, num_pattern_lengths, algorithms->num_algos, opts->num_runs);
    allocate_pattern_matrix(pattern_list, opts->num_runs, opts->pattern_info.pattern_max_len);

    for (int m = opts->pattern_info.pattern_min_len, patt_len_idx = 0; m <= max_pattern_length;
             m = next_pattern_length(&(opts->pattern_info), m), patt_len_idx++)
    {
        gen_patterns(opts, pattern_list, m, T, n, opts->num_runs);
        results[patt_len_idx].pattern_length = m;
        benchmark_algos_with_patterns(results[patt_len_idx].algo_results, opts, T, n, pattern_list, m, algorithms);
    }

    output_benchmark_run_summary(smart_config, opts, n);
    output_benchmark_statistics_csv(smart_config, opts, num_pattern_lengths, results, algorithms, n);

    free_pattern_matrix(pattern_list, opts->num_runs);
    free_benchmark_results(results, num_pattern_lengths, algorithms->num_algos);
}

/*
 * Prints an information message to explain what the timings reported by the benchmarks mean.
 */
void print_search_and_preprocessing_time_info(run_command_opts_t *opts)
{
    if (opts->pre)
    {
        info("Timings are reported for both pre-processing and search times separately.  Run without the %s option to get the total times.\n", FLAG_SHORT_PREPROCESSING_TIME);
    }
    else
    {
        info("Timings reported are the sum of pre-processing and search times.  Use the %s option to report separate times.\n", FLAG_SHORT_PREPROCESSING_TIME);
    }
}

/*
 * Prints a message if cpu statistics are being gathered.
 */
void print_cpu_stats_info(run_command_opts_t *opts)
{
    if (opts->cpu_stats)
    {
        info("CPU statistics will be captured during algorithm runs, if supported.");
    }
}

/*
 * Loads the passing test status of the algorithms, and prints a warning if some of them have not passed.
 */
void load_test_status(const smart_config_t *smart_config, algo_info_t *algorithms)
{
    if (!set_passing_test_status(smart_config, algorithms))
    {
        warn("Some algorithms have not passed testing.  These are flagged with a * and their name is in lower case.\n");
    }
}

/*
 * Loads the text and algorithms to use and then runs benchmarking.
 */
void load_and_run_benchmarks(const smart_config_t *smart_config, run_command_opts_t *opts, algo_info_t *algorithms)
{
    // Get the text to search in.
    unsigned char *T = (unsigned char *)malloc(get_text_buffer_size(opts->text_size, opts->pattern_info.pattern_max_len));
    int n = get_text(smart_config, opts, T);
    print_text_info(T, n);

    // Load the algorithm shared object libraries to benchmark.
    sort_algorithm_names(algorithms);
    load_algo_shared_libraries(smart_config, algorithms);

    print_search_and_preprocessing_time_info(opts);
    print_cpu_stats_info(opts);

    // Determine the test status of the algorithms to benchmark:
    load_test_status(smart_config, algorithms);

    // Benchmark the algorithms:
    char time_format[26];
    set_time_string(time_format, 26, "%Y:%m:%d %H:%M:%S");
    info("Experimental tests with code %s started on %s", opts->expcode, time_format);

    benchmark_algorithms_with_text(smart_config, opts, T, n, algorithms);

    set_time_string(time_format, 26, "%Y:%m:%d %H:%M:%S");
    info("Experimental tests with code %s finished on %s", opts->expcode, time_format);

    // Unload search algorithms and free text.
    unload_algos(algorithms);
    free(T);
}

/*
 * Loads the algorithms to benchmark given the run options and config.
 */
void get_algorithms_to_benchmark(const smart_config_t *smart_config, run_command_opts_t *opts, algo_info_t *algorithms)
{
    init_algo_info(algorithms);

    switch (opts->algo_source)
    {
        case ALGO_REGEXES:
        {
            get_all_algo_names(smart_config, algorithms);
            filter_out_names_not_matching_regexes(algorithms, NULL, opts->algo_names, opts->num_algo_names);
            break;
        }
        case NAMED_SET_ALGOS:
        {
            read_algo_names_from_file(smart_config, algorithms, opts->algo_filename);
            if (opts->num_algo_names > 0)
            {
                algo_info_t regex_algos;
                init_algo_info(&regex_algos);
                get_all_algo_names(smart_config, &regex_algos);
                filter_out_names_not_matching_regexes(&regex_algos, NULL, opts->algo_names, opts->num_algo_names);
                merge_algorithms(algorithms, &regex_algos, NULL);
            }
            break;
        }
        case SELECTED_ALGOS:
        {
            read_algo_names_from_file(smart_config, algorithms, opts->algo_filename);
            break;
        }
        case ALL_ALGOS:
        {
            get_all_algo_names(smart_config, algorithms);
            break;
        }
        default:
        {
            error_and_exit("Unknown algorithm source specified: %d", opts->algo_source);
        }
    }

    sort_algorithm_names(algorithms);
    print_algorithms_as_list("\tBenchmarking ", algorithms);
}

/*
 * Prints an appropriate error message if no algorithms can be found to run with.
 */
void print_algorithm_missing_error_and_exit(const smart_config_t *smart_config, run_command_opts_t *opts)
{
    switch (opts->algo_source)
    {
        case ALGO_REGEXES:
        {
            error_and_exit("No algorithms matched the ones specified on the command line.");
        }
        case ALL_ALGOS:
        {
            error_and_exit("No algorithms could be located on the algorithm search paths.");
        }
        case NAMED_SET_ALGOS:
        case SELECTED_ALGOS:
        {
            error_and_exit("No algorithms were found to benchmark in %s/%s", smart_config->smart_config_dir,
                           opts->algo_filename);
        }
        default:
        {
            error_and_exit("Unknown algorithm source %d", opts->algo_source);
        }
    }
}

/*
 * Executes the benchmark with the given benchmark options.
 */
void run_benchmark(const smart_config_t *smart_config, run_command_opts_t *opts)
{
    // Get the selected algorithm names to benchmark
    algo_info_t algorithms;
    get_algorithms_to_benchmark(smart_config, opts, &algorithms);

    if (algorithms.num_algos > 0)
    {
        load_and_run_benchmarks(smart_config, opts, &algorithms);
    }
    else
    {
        print_algorithm_missing_error_and_exit(smart_config, opts);
    }
}

/*
 * Main method to set things up before executing benchmarks with the benchmarking options provided.
 * Returns 0 if successful.
 */
int exec_run(run_command_opts_t *opts, smart_config_t *smart_config)
{
    print_logo();

    set_random_seed(opts->random_seed);

    pin_to_one_CPU_core(opts->cpu_pinning, opts->cpu_to_pin, "Variation in benchmarking may be higher.");

    run_benchmark(smart_config, opts);

    return 0;
}