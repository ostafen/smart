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
#include "parser.h"

#define PATTERN_SIZE_MAX 4200 // maximal length of the pattern

#define SIGMA 256 // constant alphabet size
#define NUM_PATTERNS_MAX 100
#define NumSetting 15 // number of text buffers

#define TEXT_SIZE_DEFAULT 1048576

#define TOP_EDGE_WIDTH 60

#define MAX_FILES 500
#define MAX_SEARCH_PATHS 50
#define MAX_LINE_LEN 128

unsigned int MINLEN = 1, MAXLEN = 4200; // min length and max length of pattern size

void print_edge(int len)
{
    int i;
    fprintf(stdout, "\t");
    for (i = 0; i < len; i++)
        fprintf(stdout, "%c", '_');
    fprintf(stdout, "\n");
}

void print_percentage(int perc)
{
    if (perc < 10 || perc > 100)
        printf("\b\b\b\b[%d%%]", perc);
    else if (perc < 100)
        printf("\b\b\b\b\b[%d%%]", perc);
    fflush(stdout);
}

/*
 * Loads an individual file given in filename into a buffer, up to a max size of n.
 * Returns the number of characters read from the file.
 */
int load_text_buffer(const char *filename, unsigned char *buffer, int n)
{
    printf("\tLoading the file %s\n", filename);

    FILE *input = fopen(filename, "r");
    if (input == NULL)
        return -1;

    int i = 0, c;
    while (i < n && (c = getc(input)) != EOF)
        buffer[i++] = c;

    fclose(input);
    return i;
}

/*
 * Loads the files defined in the list of filenames into a text buffer T, up to a maximum size of text.
 * Returns the current size of data loaded.
 */
int merge_text_buffers(const char filenames[][STR_BUF], int n_files, unsigned char *T, int max_text_size)
{
    int curr_size = 0;
    for (int i = 0; i < n_files && curr_size < max_text_size; i++)
    {
        int size = load_text_buffer(filenames[i], T + curr_size, max_text_size - curr_size);

        if (size < 0)
            return 0;

        curr_size += size;
    }

    return curr_size;
}

/*
 * Replicates existing data in a buffer to fill up any remaining space in the buffer.
 */
void replicate_buffer(unsigned char *buffer, int size, int target_size)
{
    while (size < target_size)
    {
        int cpy_size = (target_size - size) < size ? (target_size - size) : size;
        memcpy(buffer + size, buffer, cpy_size);
        size += cpy_size;
    }
}

/*
 * Adds a file or the files in a directory defined in the data source to a list of filenames,
 * searching for them in the search paths defined.
 * If the data source points to a directory, all the files in that directory are added.
 * If the data source points to a file, just that file is added.
 * Returns the number of files in the filenames array.
 */
int add_files(char search_paths[][STR_BUF], int num_search_paths, const char *data_source,
              char filenames[][STR_BUF], int num_files, int max_files)
{
    char valid_path[STR_BUF];
    locate_file_path(valid_path, data_source, search_paths, num_search_paths);

    if (valid_path[0] != '\0')
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
int build_list_of_files_to_load(const char *search_path, const char *data_sources[MAX_DATA_SOURCES], char filenames[][STR_BUF])
{
    char search_paths[MAX_SEARCH_PATHS][STR_BUF];
    int num_search_paths = split_search_paths(search_path, search_paths, MAX_SEARCH_PATHS);

    int source_index = 0, num_files = 0;
    while (source_index < MAX_DATA_SOURCES && num_files < MAX_FILES && data_sources[source_index] != NULL)
    {
        num_files = add_files(search_paths, num_search_paths, data_sources[source_index++],
                              filenames, num_files, MAX_FILES);
    }

    return num_files;
}

/*
 * Prints an error if no data could be found and exits.
 */
void print_no_data_found_and_exit(const char *search_path)
{
    char working_dir[STR_BUF];
    if (getcwd(working_dir, STR_BUF) != 0)
    {
       strcpy(working_dir, "(unknown)");
    }

    print_format_error_message_and_exit("\nERROR: No files could be found to generate the search text.\nSearch path: %s\nWorking dir: %s\n",
                                        search_path, working_dir);
}

/*
 * Loads all the files located in an array of data_sources into a buffer up to the bufsize.
 * Uses the search path to locate files or directories if they do not exist locally.
 * Returns the size of data loaded into the buffer.
 */
int gen_search_text(const char *search_path, const char *data_sources[MAX_DATA_SOURCES], unsigned char *buffer, int bufsize, int fill_buffer)
{
    char filenames[MAX_FILES][STR_BUF];
    int n_files = build_list_of_files_to_load(search_path, data_sources, filenames);
    if (n_files > 0)
    {
        int n = merge_text_buffers(filenames, n_files, buffer, bufsize);

        if (n >= bufsize || !fill_buffer)
            return n;

        replicate_buffer(buffer, n, bufsize);

        if (n > 0)
            return bufsize;
    }

    print_no_data_found_and_exit(search_path);
}

/*
 * Generates a random text and stores it in the buffer of size bufsize, with an alphabet of sigma.
 * Returns the size of the random data (which will be bufsize).
 */
int gen_random_text(const int sigma, unsigned char *buffer, const int bufsize)
{
    printf("\n\tGenerating random text with alphabet size of %d\n", sigma);

    // An alphabet of one means all symbols are the same - so just set zero.
    if (sigma == 1)
    {
        memset(buffer, 0, bufsize);
    }
    else
    {
        for (int i = 0; i < bufsize; i++)
        {
            buffer[i] = rand() % sigma;
        }
    }
    return bufsize;
}

/*
 * Generates a list of random patterns of size m by randomly extracting them from a text T of size n.
 */
void gen_random_patterns(unsigned char **patterns, int m, const unsigned char *T, int n, int num_patterns)
{
    for (int i = 0; i < num_patterns; i++)
    {
        int k = rand() % (n - m);
        for (int j = 0; j < m; j++)
            patterns[i][j] = T[k + j];
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
        return (T[n/2] + T[n/2 + 1]) / 2; // return mean of n/2 and n/2+1 elements.
    }
    else {
        return T[(n+1)/2]; // return the element in the middle of the sorted array.
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

#define SEARCH_FUNC_NAME "internal_search"


int load_algo_names_from_file(char algo_names[][STR_BUF], const char *filename)
{
    FILE *algo_file = fopen(filename, "r");
    int num_running = read_all_lines(algo_file, algo_names);
    fclose(algo_file);
    qsort(algo_names, num_running, sizeof(char) * STR_BUF, str_compare);
    return num_running;
}

/*
 * Dynamically loads the algorithms defined in algo_names as shared objects into the benchmarking process.
 * Returns 0 if successful.  Will exit with status 1 if it is unable to load an algorithm.
 */
int load_algos(const char algo_names[][STR_BUF], int num_algos, int (**functions)(unsigned char *, int, unsigned char *, int, double *, double *),
               long shared_object_handles[MAX_SELECT_ALGOS])
{
    for (int i = 0; i < num_algos; i++)
    {
        char algo_lib_filename[STR_BUF];
        //TODO: configure location of algo shared objects - environment variable?
        snprintf(algo_lib_filename, STR_BUF, "bin/algos/%s.so", str2lower((char *)algo_names[i]));

        void *lib_handle = dlopen(algo_lib_filename, RTLD_NOW);
        int (*search)(unsigned char *, int, unsigned char *, int, double *, double *) = dlsym(lib_handle, SEARCH_FUNC_NAME);
        if (lib_handle == NULL || search == NULL)
        {
            print_format_error_message_and_exit("unable to load algorithm %s\n", algo_names[i]);
        }
        shared_object_handles[i] = (long) lib_handle;
        functions[i] = search;
    }

    return 0;
}

/*
 * Closes all the dynamically loaded algorithm shared object handles.
 */
void unload_algos(int num_algos, long shared_object_handles[MAX_SELECT_ALGOS])
{
    for (int i = 0; i < num_algos; i++)
    {
        dlclose((void *) shared_object_handles[i]);
    }
}

void allocate_pattern_matrix(unsigned char **M, int n, int elementSize)
{
    for (int i = 0; i < n; i++)
        M[i] = (unsigned char *)malloc(elementSize);
}

void free_pattern_matrix(unsigned char **M, int n)
{
    for (int i = 0; i < n; i++)
        free(M[i]);
}

enum measurement_status {SUCCESS, TIMED_OUT, CANNOT_SEARCH, ERROR};

typedef struct algo_measurements
{
    double *search_times;
    double *pre_times;
} algo_measurements_t;

typedef struct algo_statistics
{
    double mean_search_time;
    double median_search_time;
    double std_search_time;
    double mean_pre_time;
    double median_pre_time;
} algo_statistics_t;

typedef struct algo_results
{
    int algo_id;
    enum measurement_status success_state;
    algo_statistics_t statistics;
    int occurrence_count;
    algo_measurements_t measurements;
} algo_results_t;

typedef struct benchmark_results
{
    int pattern_length;
    algo_results_t *algo_results;
} benchmark_results_t;

void allocate_benchmark_results(benchmark_results_t *bench_result, int num_pattern_lengths, int num_algos, int num_runs)
{
    // For each pattern length we process, allocate space for bench_result for all the algorithms:
    for (int i = 0; i < num_pattern_lengths; i++)
    {
        bench_result[i].algo_results = malloc(sizeof(algo_results_t) * num_algos);

        //For each algorithm we process for a pattern length, allocate space for all the measurements it will make:
        for (int j = 0; j < num_algos; j++)
        {
            bench_result[i].algo_results[j].measurements.search_times = malloc(sizeof(double) * num_runs);
            bench_result[i].algo_results[j].measurements.pre_times = malloc(sizeof(double) * num_runs);
        }
    }
}

void free_benchmark_results(benchmark_results_t *bench_result, int num_pattern_lengths, int num_algos)
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
 * Returns a status code: 0 if successful, -1 if the algorithm will not run and -2 if the algorithm timed out.
 */
enum measurement_status run_algo(unsigned char **pattern_list, int m,
                                 unsigned char *T, int n, const run_command_opts_t *opts,
                                 int (*search_func)(unsigned char *, int, unsigned char *, int, double *, double *), algo_results_t *results)
{
    unsigned char P[m + 1];
    results->occurrence_count = 0;

    for (int k = 0; k < opts->num_runs; k++)
    {
        print_percentage((100 * (k + 1)) / opts->num_runs);

        memcpy(P, pattern_list[k], sizeof(unsigned char) * (m + 1));

        results->measurements.pre_times[k] = results->measurements.search_times[k] = 0.0;

        int occur = search_func(P, m, T, n, results->measurements.search_times[k], results->measurements.pre_times[k]);

        results->measurements.pre_times[k] = pre_time;
        results->measurements.search_times[k] = search_time;

        if (occur == 0)
            return ERROR; // there must be at least one match for each text and pattern (patterns are extracted from text).

        if (occur < 0)
            return CANNOT_SEARCH; // negative value returned from search means it cannot search this pattern.

        results->occurrence_count += occur;

        if (search_time > opts->time_limit_millis)
            return TIMED_OUT;
    }

    return SUCCESS;
}

/*
 * Prints benchmark results for an algorithm run.
 */
void print_benchmark_res(char *output_line, const run_command_opts_t *opts, algo_results_t *results)
{
    switch (results->success_state)
    {
        case SUCCESS:
        {
            printf("\b\b\b\b\b\b\b.[OK]  ");
            if (opts->pre)
                snprintf(output_line, MAX_LINE_LEN,"\tmean: %.2f + [%.2f ± %.2f] ms\tmedian: %.2nf + [%.2f] ms",
                        results->statistics.mean_pre_time,
                        results->statistics.mean_search_time,
                        results->statistics.std_search_time,
                        results->statistics.median_pre_time,
                        results->statistics.median_search_time);
            else //TODO: we are not adding pre-time to search time.  should we get rid of -pre option entirely and always report pre-time?
                snprintf(output_line, MAX_LINE_LEN,"\tmean: [%.2f ± %.2f] ms\tmedian: %.2f ms",
                        results->statistics.mean_search_time,
                        results->statistics.std_search_time,
                        results->statistics.median_search_time);

            printf("%s", output_line);

            if (opts->occ)
            {
                if (opts->pre)
                    printf("\t\tocc \%d", results->occurrence_count);
                else
                    printf("\tocc \%d", results->occurrence_count);
            }
            printf("\n");
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

int double_compare(const void *a, const void *b)
{
    return (*(double*)a > *(double*)b) ? 1 : (*(double*)a < *(double*)b) ? -1 : 0;
}

void calculate_algo_statistics(algo_results_t *results, int num_measurements)
{
    // Compute mean pre and search times:
    results->statistics.mean_pre_time    = compute_average(results->measurements.pre_times, num_measurements);
    results->statistics.mean_search_time = compute_average(results->measurements.search_times, num_measurements);
    results->statistics.std_search_time  = compute_std(results->statistics.mean_search_time,
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



int benchmark_algos_using_random_patterns(algo_results_t *results, const run_command_opts_t *opts, unsigned char *T, int n, unsigned char **pattern_list, int m,
                                          int num_running, char algo_names[][STR_BUF], int (**algo_functions)(unsigned char *, int, unsigned char *, int))
{
    printf("\n");
    print_edge(TOP_EDGE_WIDTH);

    printf("\tSearching for a set of %d patterns with length %d\n", opts->num_runs, m);
    printf("\tTesting %d algorithms\n", num_running);
    printf("\n");

    int current_running = 0;
    for (int algo = 0; algo < num_running; algo++)
    {
        current_running++;

        char output_line[MAX_LINE_LEN];
        snprintf(output_line, MAX_LINE_LEN, "\t - [%d/%d] %s ", current_running, num_running, str2upper(algo_names[algo]));
        printf("%s", output_line);
        fflush(stdout);

        for (int i = 0; i < 35 - strlen(output_line); i++)
            printf(".");

        results->algo_id = algo;
        results->success_state = run_algo(pattern_list, m, T, n, opts, algo_functions[algo], results);

        if (results->success_state == SUCCESS)
            calculate_algo_statistics(results, opts->num_runs);

        print_benchmark_res(output_line, opts, results);
    }
}

int get_num_pattern_lengths(const run_command_opts_t *opts)
{
    int num_patterns = 0;
    int value = opts->pattern_min_len;
    while (value <= opts->pattern_max_len && num_patterns <= NUM_PATTERNS_MAX)
    {
        value *= 2; // we can change this calculation to support arithmetic progressions as well as geometric.
        num_patterns++;
    }
    return num_patterns;
}

int benchmark_algos(const char *expcode, const run_command_opts_t *opts, unsigned char *T, int n,
                    int num_algos, char algo_names[][STR_BUF], int (**algo_functions)(unsigned char *, int, unsigned char *, int))
{
    int num_pattern_lengths = get_num_pattern_lengths(opts);
    printf("num pattern lengths = %d", num_pattern_lengths);
    benchmark_results_t results[num_pattern_lengths];
    unsigned char *pattern_list[opts->num_runs];

    allocate_benchmark_results(results, num_pattern_lengths, num_algos, opts->num_runs);
    allocate_pattern_matrix(pattern_list, opts->num_runs, sizeof(unsigned char) * (PATTERN_SIZE_MAX + 1));

    //TODO: can replace this hard-coded power of two thing with configurable arithmetic or geometric progressions.
    //      e.g. set lower bound, upper bound, operator and ratio/step - e.g. (1-16 + 1) or (2-156 * 1.5).
    for (int m = opts->pattern_min_len, pattern_idx = 0; m <= opts->pattern_max_len; m *= 2, pattern_idx++)
    {
        gen_random_patterns(pattern_list, m, T, n, opts->num_runs);
        results[pattern_idx].pattern_length = m;
        benchmark_algos_using_random_patterns(results[pattern_idx].algo_results, opts, T, n, pattern_list, m, num_algos, algo_names, algo_functions);
    }

    free_pattern_matrix(pattern_list, opts->num_runs);
    free_benchmark_results(results, num_pattern_lengths, num_algos);
    return 0;
}

/*
 * Generates a code to identify each local benchmark experiment that is run.
 * The code is not guaranteed to be globally unique - it is simply based on the local time a benchmark was run.
 */
void gen_experiment_code(char *code, int max_len)
{
    snprintf(code, max_len, "EXP%d", (int)time(NULL));
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
    printf("\tText buffer of dimension %d byte\n", n);

    int freq[SIGMA];
    compute_frequency(T, n, freq);

    int alphabet_size, max_code;
    compute_alphabet_info(freq, &alphabet_size, &max_code);

    printf("\tAlphabet of %d characters.\n", alphabet_size);
    printf("\tGreater chararacter has code %d.\n", max_code);
}

/*
 * Fills the text buffer T with opts->text_size of data.
 * Returns the size of the text loaded.
 */
int get_text(run_command_opts_t *opts, unsigned char *T)
{
    int size = 0;
    if (opts->data_source == RANDOM)
    {
        size = gen_random_text(opts->alphabet_size, T, opts->text_size);
    }
    else if (opts->data_source == FILES)
    {
        size = gen_search_text(opts->search_path, opts->data_sources, T, opts->text_size, opts->fill_buffer);
    }
    else
    {
        print_error_message_and_exit("Undefined source for data.");
    }
    return size;
}

/*
 * Executes the benchmark with the given benchmark options with a text buffer T.
 * Size of the T buffer is opts->text_size + PATTERN_SIZE_MAX to allow algorithms to place a copy of the pattern
 * at the end of the text if they wish.  This is a special optimisation that allows a search algorithm to omit a length check,
 * as the algorithm is guaranteed to stop when it detects the sentinel pattern past the end of the actual text.
 */
void run_benchmark(run_command_opts_t *opts, unsigned char *T)
{
    // Generate experiment code
    char expcode[STR_BUF];
    gen_experiment_code(expcode, STR_BUF);
    printf("\tStarting experimental tests with code %s\n", expcode);

    // Get the text to search in:
    int n = get_text(opts, T);
    print_text_info(T, n);

    // Load the algorithms to search with:
    char algo_names[MAX_SELECT_ALGOS][STR_BUF];
    int (*algo_functions[MAX_SELECT_ALGOS])(unsigned char *, int, unsigned char *, int);
    long shared_object_handles[MAX_SELECT_ALGOS];
    int num_running = load_algo_names_from_file(algo_names, "selected_algos");
    load_algos(algo_names, num_running, algo_functions, shared_object_handles);

    // Benchmark the algorithms:
    char time_format[26];
    set_time_string(time_format, 26, "%Y:%m:%d %H:%M:%S");
    printf("\tExperimental tests started on %s\n", time_format);
    benchmark_algos(expcode, opts, T, n, num_running, algo_names, algo_functions);
    printf("\n");

    // Unload search algorithms.
    unload_algos(num_running, shared_object_handles);

    // outputINDEX(filename_list, num_buffers, expcode);
}

/*
 * Sets the scheduler affinity to pin this process to one CPU core.
 * This can avoid benchmarking variation caused by processes moving from one core to another, causing cache misses.
 */
void pin_to_one_CPU_core(run_command_opts_t *opts)
{
    if (!strcmp(opts->cpu_pinning, "off"))
    {
        printf("\tCPU pinning not enabled: variation in benchmarking may be higher.\n\n");
    }
    else
    {
        int num_processors = (int)sysconf(_SC_NPROCESSORS_ONLN);
        int cpu_to_pin;
        if (!strcmp(opts->cpu_pinning, "last"))
        {
            cpu_to_pin = num_processors - 1;
        }
        else
        {
            cpu_to_pin = atoi(opts->cpu_pinning);
        }

        if (cpu_to_pin < num_processors)
        {
            int pid = getpid();
            cpu_set_t cpus;
            CPU_ZERO(&cpus);
            CPU_SET(cpu_to_pin, &cpus);

            if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpus) == -1)
            {
                printf("\tCould not pin the benchmark to a core: variation in benchmarking may be higher.\n\n");
            }
            else
            {
                printf("\tPinned benchmark process %d to core %d of 0 - %d processors.\n\n", pid, cpu_to_pin, num_processors - 1);
            }
        }
        else
        {
            printf("\tCould not pin cpu %d to available cores 0 - %d: variation in benchmarking may be higher.\n\n", cpu_to_pin, num_processors - 1);
        }
    }
}

/*
 * Sets the random seed for this run from the seed defined in opts.
 */
void set_random_seed(run_command_opts_t *opts)
{
    srand(opts->random_seed);
    printf("\n\tSetting random seed for this benchmarking run to %ld.  Use -seed %ld if you need to rerun identically.\n",
           opts->random_seed, opts->random_seed);
}

/*
 * Main method to set things up before executing benchmarks with the benchmarking options provided.
 * Returns 0 if successful.
 */
int exec_run(run_command_opts_t *opts)
{
    print_logo();

    set_random_seed(opts);

    pin_to_one_CPU_core(opts);

    unsigned char *T = (unsigned char *)malloc(sizeof(unsigned char) * (opts->text_size + PATTERN_SIZE_MAX));

    run_benchmark(opts, T);

    free(T);

    return 0;
}