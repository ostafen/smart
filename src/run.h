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

#include <sched.h>

#define PATTERN_SIZE_MAX 4200 // maximal length of the pattern

#define SIGMA 256 // constant alphabet size
#define NUM_PATTERNS_MAX 100
#define NumSetting 15 // number of text buffers

#define TEXT_SIZE_DEFAULT 1048576

#define TOP_EDGE_WIDTH 60

#define MAX_FILES 500
#define MAX_SEARCH_PATHS 50

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
    if (perc < 10)
        printf("\b\b\b\b[%d%%]", perc);
    else if (perc < 100)
        printf("\b\b\b\b\b[%d%%]", perc);
    else
        printf("\b\b\b\b[%d%%]", perc);
    fflush(stdout);
}

/*
 * Loads an individual file given in filename into a buffer, up to a max size of n.
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

        //TODO: check this logic - when can size be less than zero?
        if (size < 0)
            return 0;

        curr_size += size;
    }

    //TODO: why set a text end in the "text" buffer?  It can contain binary data that has zeros in it anyway.  it's not really text (or it doesn't have to be).
    T[curr_size < max_text_size ? curr_size : max_text_size - 1] = '\0';

    return curr_size;
}

/*
 * Replicates existing data in a buffer to fill up any remaining space in the buffer.
 */
int replicate_buffer(unsigned char *buffer, int size, int target_size)
{
    while (size < target_size)
    {
        int cpy_size = (target_size - size) < size ? (target_size - size) : size;
        memcpy(buffer + size, buffer, cpy_size);
        size += cpy_size;
    }
    buffer[size - 1] = '\0';
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
        if (S_ISDIR(file_mode)) {
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
    if (n_files > 0) {
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
 * Returns the total size generated.
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
void gen_random_patterns(unsigned char **patterns, int m, unsigned char *T, int n, int num_patterns)
{
    for (int i = 0; i < num_patterns; i++)
    {
        int k = random() % (n - m);
        for (int j = 0; j < m; j++)
            patterns[i][j] = T[k + j];

        patterns[i][m] = '\0';
    }
}

/*
 * Computes the mean average of a list of search times of size n.
 */
double compute_average(double *T, int n)
{
    double avg = 0.0;
    for (int i = 0; i < n; i++)
        avg += T[i];

    return avg / n;
}

/*
 * Computes the standard deviation given a mean average, and a list of search times of size n.
 */
double compute_std(double avg, double *T, int n)
{
    double std = 0.0;
    for (int i = 0; i < n; i++)
        std += pow(avg - T[i], 2.0);

    return sqrt(std / n);
}

#define SEARCH_FUNC_NAME "search"

/*
 * Dynamically loads the algorithms defined in algo_names as shared objects into the benchmarking process.
 */
// TODO: dlclose() all lib_handle pointers
int load_algos(const char algo_names[][STR_BUF], int num_algos, int (**functions)(unsigned char *, int, unsigned char *, int))
{
    for (int i = 0; i < num_algos; i++)
    {
        char algo_lib_filename[STR_BUF];
        //TODO: configure location of algo shared objects - environment variable?
        sprintf(algo_lib_filename, "bin/algos/%s.so", str2lower((char *)algo_names[i]));

        void *lib_handle = dlopen(algo_lib_filename, RTLD_NOW);
        int (*search)(unsigned char *, int, unsigned char *, int) = dlsym(lib_handle, SEARCH_FUNC_NAME);
        if (lib_handle == NULL || search == NULL)
        {
            printf("unable to load algorithm %s\n", algo_names[i]);
            exit(1);
        }
        functions[i] = search;
    }
    return 0;
}

void allocate_matrix(unsigned char **M, int n, int elementSize)
{
    for (int i = 0; i < n; i++)
        M[i] = (unsigned char *)malloc(elementSize);
}

void free_matrix(unsigned char **M, int n)
{
    for (int i = 0; i < n; i++)
        free(M[i]);
}

double search_time, pre_time;

typedef struct bechmark_res
{
    int total_occ;
    double search_time, pre_time, std;
} benchmark_res_t;

/*
 * Benchmarks an algorithm against a list of patterns of size m on a text T of size n, using the options provided.
 */
int run_algo(unsigned char **pattern_list, int m,
              unsigned char *T, int n, const run_command_opts_t *opts,
              int (*search_func)(unsigned char *, int, unsigned char *, int), benchmark_res_t *res)
{
    double times[opts->num_runs];
    double total_pre_time = 0;

    res->total_occ = 0;
    for (int k = 0; k < opts->num_runs; k++)
    {
        print_percentage((100 * (k + 1)) / opts->num_runs);

        unsigned char P[m + 1];
        memcpy(P, pattern_list[k], sizeof(unsigned char) * (m + 1));
        res->search_time = res->pre_time = 0.0;

        int occur = search_func(P, m, T, n);
        res->total_occ += occur;

        if (occur <= 0)
            return -1;

        if (search_time > opts->time_limit_millis)
            return -2;

        times[k] = search_time;
        total_pre_time += pre_time;
    }

    res->search_time = compute_average(times, opts->num_runs);
    res->pre_time = total_pre_time / opts->num_runs;
    res->std = compute_std(res->search_time, times, opts->num_runs);
}

/*
 * Prints benchmark results for an algorithm run.
 */
void print_benchmark_res(char *output_line, const run_command_opts_t *opts, benchmark_res_t *res)
{
    if (res->total_occ > 0)
    {
        printf("\b\b\b\b\b\b\b.[OK]  ");
        if (opts->pre)
            sprintf(output_line, "\t%.2f + [%.2f ± %.2f] ms", res->pre_time, res->search_time, res->std);
        else
            sprintf(output_line, "\t[%.2f ± %.2f] ms", res->search_time, res->std);

        printf("%s", output_line);
        if (opts->occ)
        {
            if (opts->pre)
                printf("\t\tocc \%d", res->total_occ / opts->num_runs);
            else
                printf("\tocc \%d", res->total_occ / opts->num_runs);
        }
        printf("\n");
    }
    else if (res->total_occ == 0)
        printf("\b\b\b\b\b\b\b\b.[ERROR] \n");
    else if (res->total_occ == -1)
        printf("\b\b\b\b\b.[--]  \n");
    else if (res->total_occ == -2)
        printf("\b\b\b\b\b\b.[OUT]  \n");
}

/*
 * Runs all the benchmarks for the selected algorithms given the run options, and a text buffer T of size n.
 * The size n refers to the maximum size of the text to search; the buffer itself is slightly bigger to allow copying a
 * pattern into it past the actual text.
 */
int run_setting(unsigned char *T, int n, const run_command_opts_t *opts)
{
    unsigned char *pattern_list[opts->num_runs];
    allocate_matrix(pattern_list, opts->num_runs, sizeof(unsigned char) * (PATTERN_SIZE_MAX + 1));

    FILE *algo_file = fopen("selected_algos", "r");
    char algo_names[MAX_SELECT_ALGOS][STR_BUF];
    int num_running = read_all_lines(algo_file, algo_names);
    qsort(algo_names, num_running, sizeof(char) * STR_BUF, str_compare);

    int (*algo_functions[MAX_SELECT_ALGOS])(unsigned char *, int, unsigned char *, int);
    load_algos(algo_names, num_running, algo_functions);

    // initializes the vector which will contain running times
    // performs experiments on a text
    double SEARCH_TIME[num_running][NUM_PATTERNS_MAX], PRE_TIME[num_running][NUM_PATTERNS_MAX];
    for (int i = 0; i < num_running; i++)
        for (int j = 0; j < NUM_PATTERNS_MAX; j++)
            SEARCH_TIME[i][j] = PRE_TIME[i][j] = 0;

    for (int m = opts->pattern_min_len, pattern_idx = 0; m <= opts->pattern_max_len; m *= 2, pattern_idx++)
    {
        gen_random_patterns(pattern_list, m, T, n, opts->num_runs);

        printf("\n");
        print_edge(TOP_EDGE_WIDTH);

        printf("\tSearching for a set of %d patterns with length %d\n", opts->num_runs, m);
        printf("\tTesting %d algorithms\n", num_running);
        printf("\n");

        int current_running = 0;
        for (int algo = 0; algo < num_running; algo++)
        {
            current_running++;

            char output_line[30];
            sprintf(output_line, "\t - [%d/%d] %s ", current_running, num_running, str2upper(algo_names[algo]));
            printf("%s", output_line);
            fflush(stdout);

            for (int i = 0; i < 35 - strlen(output_line); i++)
                printf(".");

            double TIME[opts->num_runs];
            int total_pre_time = 0;
            benchmark_res_t res;
            run_algo(pattern_list, m, T, n, opts, algo_functions[algo], &res);

            print_benchmark_res(output_line, opts, &res);

            // save times for outputting if necessary
            SEARCH_TIME[algo][pattern_idx] = 0;
            PRE_TIME[algo][pattern_idx] = 0;
            if (res.total_occ > 0)
            {
                SEARCH_TIME[algo][pattern_idx] = res.search_time;
                PRE_TIME[algo][pattern_idx] = res.pre_time;
            }
        }
    }
    printf("\n");

    // free memory allocated for patterns
    free_matrix(pattern_list, opts->num_runs);

    return 0;
}

/*
 * Generates a code to identify each local benchmark experiment that is run.
 * The code is not guaranteed to be globally unique - it is simply based on the local time a benchmark was run.
 */
void gen_experiment_code(char *code)
{
    sprintf(code, "EXP%d", (int)time(NULL));
}

/*
 * Computes information about the size of the alphabet contained in character frequency table freq, and gives
 * both the number of unique characters, and the maximum character code it contains.
 */
void compute_alphabet_info(int *freq, int *alphabet_size, int *max_code)
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
    int nalpha = 0;
    int maxcode = 0;
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
 * The filename either specifies the string defined in SMART_RANDOM_TEXT, in which case randomised text
 * will be generated using the alphabet size in opts->alphabet_size.
 * Otherwise, the filename should be a path to a directory that contains the text files to load (up to the buffer
 * size limit).
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
    char expcode[STR_BUF];
    gen_experiment_code(expcode);
    printf("\tStarting experimental tests with code %s\n", expcode);

    int n = get_text(opts, T);
    print_text_info(T, n);

    time_t date_timer;
    char time_format[26];
    struct tm *tm_info;
    time(&date_timer);
    tm_info = localtime(&date_timer);
    strftime(time_format, 26, "%Y:%m:%d %H:%M:%S", tm_info);
    printf("\tExperimental tests started on %s\n", time_format);

    run_setting(T, n, opts);

    // outputINDEX(filename_list, num_buffers, expcode);
}

/*
 * Sets the scheduler affinity to pin this process to one CPU core.
 * This can avoid benchmarking variation caused by processes moving from one core to another, causing cache misses.
 */
void pin_to_one_CPU_core(run_command_opts_t *opts)
{
    if (!strcmp(opts->cpu_pinning, "off")) {
        printf("\tCPU pinning not enabled: variation in benchmarking may be higher.\n\n");
    } else {
        int num_processors = sysconf(_SC_NPROCESSORS_ONLN);
        int cpu_to_pin;
        if (!strcmp(opts->cpu_pinning, "last"))
        {
            cpu_to_pin = num_processors - 1;
        } else {
            cpu_to_pin = atoi(opts->cpu_pinning);
        }

        if (cpu_to_pin < num_processors) {
            int pid = getpid();
            cpu_set_t cpus;
            CPU_ZERO(&cpus);
            CPU_SET(cpu_to_pin, &cpus);

            if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpus) == -1) {
                printf("\tCould not pin the benchmark to a core: variation in benchmarking may be higher.\n\n");
            } else {
                printf("\tPinned benchmark process %d to core %d of 0 - %d processors.\n\n", pid, cpu_to_pin, num_processors - 1);
            }
        }
        else {
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