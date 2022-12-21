#include <stdlib.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "timer.h"
#include "defines.h"
#include "config.h"
#include "commands.h"
#include "algorithms.h"
#include "cpu_pinning.h"

#define TOP_EDGE_WIDTH 60

static const char *DOTS = "................................................................";

void print_edge(int len)
{
    int i;
    fprintf(stdout, "\t");
    for (i = 0; i < len; i++)
        fprintf(stdout, "%c", '_');
    fprintf(stdout, "\n");
}


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
 * Loads the files defined in the list of filenames into a text buffer T, up to a maximum size of text.
 * Returns the current size of data loaded.
 */
int merge_text_buffers(const char filenames[][MAX_PATH_LENGTH], int n_files, unsigned char *T, int max_text_size)
{
    int curr_size = 0;
    for (int i = 0; i < n_files && curr_size < max_text_size; i++)
    {
        info("Loading the file %s", filenames[i]);
        int size = load_text_buffer(filenames[i], T + curr_size, max_text_size - curr_size);

        if (size > 0)
        {
            curr_size += size;
        }
        else
        {
            warn("Could not load file: %s", filenames[i]);
        }
    }

    return curr_size;
}

/*
 * Replicates existing data of length size in a buffer to fill up any remaining space in the buffer.
 */
void replicate_buffer(unsigned char *buffer, int size, int target_size)
{
    if (size > 0)
    {
        while (size < target_size)
        {
            int cpy_size = (target_size - size) < size ? (target_size - size) : size;
            memcpy(buffer + size, buffer, cpy_size);
            size += cpy_size;
        }
    }
}

/*
 * Adds a file or the files in a directory defined in the data source to a list of filenames,
 * searching for them in the search paths defined.
 * If the data source points to a directory, all the files in that directory are added.
 * If the data source points to a file, just that file is added.
 * Returns the number of files in the filenames array.
 */
int add_files(const char search_paths[][MAX_PATH_LENGTH], int num_search_paths, const char *data_source,
              char filenames[][MAX_PATH_LENGTH], int num_files, int max_files)
{
    char valid_path[MAX_PATH_LENGTH];
    locate_file_path(valid_path, data_source, search_paths, num_search_paths);

    if (!is_empty_string(valid_path))
    {
        __mode_t file_mode = get_file_mode(valid_path);
        if (S_ISDIR(file_mode))
        {
            num_files = add_filenames_in_dir(valid_path, filenames, num_files, max_files);
        }
        else if (S_ISREG(file_mode) && num_files < max_files)
        {
            strcpy(filenames[num_files++], valid_path);
        }
    }

    return num_files;
}

/*
 * Builds a list of filenames from the data sources and places them in filenames, searching the search path.
 * Returns the number of files in the filenames array.
 */
int build_list_of_files_to_load(const smart_config_t *smart_config, const char *data_sources[MAX_DATA_SOURCES], char filenames[][MAX_PATH_LENGTH])
{
    int source_index = 0, num_files = 0;
    while (source_index < MAX_DATA_SOURCES && num_files < MAX_DATA_FILES && data_sources[source_index] != NULL)
    {
        num_files = add_files(smart_config->smart_data_search_paths,
                              smart_config->num_data_search_paths,
                              data_sources[source_index++],
                              filenames, num_files, MAX_DATA_FILES);
    }

    return num_files;
}

/*
 * Loads all the files located in an array of data_sources into a buffer up to the bufsize.
 * Uses the search path to locate files or directories if they do not exist locally.
 * Returns the size of data loaded into the buffer.
 */
int gen_search_text(const smart_config_t *smart_config, const char *data_sources[MAX_DATA_SOURCES], unsigned char *buffer, int bufsize, int fill_buffer)
{
    char filenames[MAX_DATA_FILES][MAX_PATH_LENGTH];
    int n_files = build_list_of_files_to_load(smart_config, data_sources, filenames);
    if (n_files > 0)
    {
        int n = merge_text_buffers(filenames, n_files, buffer, bufsize);

        if (n >= bufsize || !fill_buffer)
            return n;

        replicate_buffer(buffer, n, bufsize);

        if (n > 0)
            return bufsize;
    }

    error_and_exit("No files could be found to generate the search text.");
}

/*
 * Loads user supplied data from the command line as the search data into the buffer.
 *
 * WARNING: the buffer must be guaranteed to be at least opts->text_size in size.
 */
int gen_user_data(const run_command_opts_t *opts, unsigned char *buffer)
{
    int size = strlen(opts->data_to_search);
    int to_copy = size < opts->text_size ? size : opts->text_size;
    memcpy(buffer, opts->data_to_search, to_copy);

    return size;
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
 * Computes and returns the median of a sorted array T of doubles of size n.
 */
double compute_median_of_sorted_array(const double *T, int n)
{
    // if the list of doubles  has an even number of elements:
    if (n % 2 == 0)
    {
        return (T[n / 2] + T[n / 2 + 1]) / 2; // return mean of n/2 and n/2+1 elements.
    }
    else
    {
        return T[(n + 1) / 2]; // return the element in the middle of the sorted array.
    }
}

/*
 * Computes and returns the standard deviation given a mean average, and a list of search times of size n.
 */
double compute_std(double avg, double *T, int n)
{
    double std = 0.0;
    for (int i = 0; i < n; i++)
        std += pow(avg - T[i], 2.0);

    return sqrt(std / n);
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
} algo_measurements_t;

/*
 * Various statistics which can be computed from the measurements of an algorithm for a single pattern length.
 */
typedef struct algo_statistics
{
    double mean_search_time;
    double median_search_time;
    double std_search_time;
    double mean_pre_time;
    double median_pre_time;
} algo_statistics_t;

/*
 * The result of benchmarking a single algorithm, including all measurements and statistics derived from them,
 * for a single length of pattern.
 */
typedef struct algo_results
{
    int algo_id;
    enum measurement_status success_state;
    algo_statistics_t statistics;
    int occurrence_count;
    algo_measurements_t measurements;
} algo_results_t;

/*
 * All measurements and statistics from benchmarking all selected algorithms for a set size of pattern.
 */
typedef struct benchmark_results
{
    int pattern_length;
    algo_results_t *algo_results;
} benchmark_results_t;

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
        }
        free(bench_result[i].algo_results);
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
    unsigned char P[m + 1];
    results->occurrence_count = 0;

    for (int k = 0; k < opts->num_runs; k++)
    {
        print_percentage((100 * (k + 1)) / opts->num_runs);

        memcpy(P, pattern_list[k], sizeof(unsigned char) * (m + 1));

        results->measurements.pre_times[k] = results->measurements.search_times[k] = 0.0;

        int occur = search_func(P, m, T, n, &(results->measurements.search_times[k]), &(results->measurements.pre_times[k]));

        if (occur == 0 || occur == ERROR_SEARCHING)
        {
            return ERROR; // there must be at least one match for each text and pattern (patterns are extracted from text).
        }

        if (occur == INFO_CANNOT_SEARCH)
            return CANNOT_SEARCH; // negative value returned from search means it cannot search this pattern (e.g. it is too short)

        results->occurrence_count += occur;

        if (results->measurements.search_times[k] > opts->time_limit_millis)
            return TIMED_OUT;
    }

    return SUCCESS;
}

/*
 * Gets the results formatted according to the run options.
 */
void get_results_info(char output_line[MAX_LINE_LEN], const run_command_opts_t *opts, algo_results_t *results)
{
    char occurence[MAX_LINE_LEN];
    if (opts->occ)
    {
        snprintf(occurence, MAX_LINE_LEN, "occ %d", results->occurrence_count);
    }
    else
    {
        occurence[0] = STR_END_CHAR;
    }

    if (opts->pre)
        snprintf(output_line, MAX_LINE_LEN, "\tmean: %.2f + [%.2f ± %.2f] ms\tmedian: %.2f + [%.2f] ms\t\t%s",
                 results->statistics.mean_pre_time,
                 results->statistics.mean_search_time,
                 results->statistics.std_search_time,
                 results->statistics.median_pre_time,
                 results->statistics.median_search_time,
                 occurence);
    else
        snprintf(output_line, MAX_LINE_LEN, "\tmean: [%.2f ± %.2f] ms\tmedian: %.2f ms\t%s",
                 results->statistics.mean_search_time + results->statistics.mean_pre_time,
                 results->statistics.std_search_time,
                 results->statistics.median_search_time + results->statistics.median_pre_time,
                 occurence);
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
 * Calculates statistics for an algorithm for a given pattern length, given the measurements taken for that
 * algorithm over a number of runs.
 */
void calculate_algo_statistics(algo_results_t *results, int num_measurements)
{
    // Compute mean pre and search times:
    results->statistics.mean_pre_time = compute_average(results->measurements.pre_times, num_measurements);
    results->statistics.mean_search_time = compute_average(results->measurements.search_times, num_measurements);
    results->statistics.std_search_time = compute_std(results->statistics.mean_search_time,
                                                      results->measurements.search_times, num_measurements);

    // Compute median pre and search times:
    // To calculate medians, we need to sort the arrays.   Copy them into a temp array before sorting,
    // otherwise we destroy the relationship between pre-time and search_time for each measurement.
    double temp[num_measurements];
    memcpy(temp, results->measurements.pre_times, sizeof(double) * num_measurements);
    qsort(temp, num_measurements, sizeof(double), double_compare);
    results->statistics.median_pre_time = compute_median_of_sorted_array(temp, num_measurements);

    memcpy(temp, results->measurements.search_times, sizeof(double) * num_measurements);
    qsort(temp, num_measurements, sizeof(double), double_compare);
    results->statistics.median_search_time = compute_median_of_sorted_array(temp, num_measurements);
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
    printf("\n");
    print_edge(TOP_EDGE_WIDTH);

    info("\tSearching for a set of %d patterns with length %d", opts->num_runs, m);
    info("\tTesting %d algorithms\n", algorithms->num_algos);

    for (int algo = 0; algo < algorithms->num_algos; algo++)
    {
        print_benchmark_status(algo, algorithms);

        results->algo_id = algo;
        results->success_state = run_algo(pattern_list, m, T, n, opts, algorithms->algo_functions[algo], results);

        if (results->success_state == SUCCESS)
            calculate_algo_statistics(results, opts->num_runs);

        print_benchmark_res(opts, results);
    }
}

/*
 * Computes information about the size of the alphabet contained in character frequency table freq, and gives
 * both the number of unique characters, and the maximum character code it contains.
 */
void compute_alphabet_info(const int *freq, int *alphabet_size, int *max_code)
{
    *alphabet_size = 0;
    *max_code = 0;
    for (int c = 0; c < SIGMA; c++)
    {
        if (freq[c] != 0)
        {
            (*alphabet_size)++;

            if (c > *max_code)
                *max_code = c;
        }
    }
}

/*
 * Computes the frequency of characters contained in text buffer T of size n, and places them in the freq table.
 */
void compute_frequency(const unsigned char *T, int n, int *freq)
{
    for (int j = 0; j < SIGMA; j++)
        freq[j] = 0;

    for (int j = 0; j < n; j++)
        freq[T[j]]++;
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
 * Benchmarks all algorithms over a text T for all pattern lengths.
 */
void benchmark_algorithms_with_text(const run_command_opts_t *opts, unsigned char *T, int n, const algo_info_t *algorithms)
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

    // Determine the test status of the algorithms to benchmark:
    load_test_status(smart_config, algorithms);

    // Benchmark the algorithms:
    char time_format[26];
    set_time_string(time_format, 26, "%Y:%m:%d %H:%M:%S");
    info("Experimental tests with code %s started on %s", opts->expcode, time_format);

    benchmark_algorithms_with_text(opts, T, n, algorithms);

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