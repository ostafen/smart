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

#define __USE_GNU // required to use CPU_ macros in sched.h.
#include <sched.h>

#define PATTERN_SIZE_MAX 4200 // maximal length of the pattern

#define SIGMA 256 // constant alphabet size
#define NUM_PATTERNS_MAX 100
#define NumSetting 15 // number of text buffers

#define TEXT_SIZE_DEFAULT 1048576

#define TOP_EDGE_WIDTH 60

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

int load_text_buffer(const char *filename, unsigned char *buffer, int n)
{
    printf("\tLoading the file %s\n", filename);

    FILE *input = fopen(filename, "r");
    if (input == NULL)
        return -1;

    int i = 0;
    char c;
    while (i < n && (c = getc(input)) != EOF)
        buffer[i++] = c;

    fclose(input);
    return i;
}

#define MAX_FILES 500
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

    T[curr_size < max_text_size ? curr_size : max_text_size - 1] = '\0';

    return curr_size;
}

size_t fsize(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return -1;
    return st.st_size;
}

int replicate_buffer(unsigned char *buffer, int size, int target_size)
{
    int i = 0;
    while (size < target_size)
    {
        int cpy_size = (target_size - size) < size ? (target_size - size) : size;
        memcpy(buffer + size, buffer, cpy_size);
        size += cpy_size;
    }
    buffer[size - 1] = '\0';
}

int gen_search_text(const char *path, unsigned char *buffer, int bufsize)
{
    char filenames[MAX_FILES][STR_BUF];

    int n_files = list_dir(path, filenames, DT_REG, 1);
    if (n_files < 0)
        return -1;

    int n = merge_text_buffers(filenames, n_files, buffer, bufsize);
    if (n >= bufsize)
        return n;

    replicate_buffer(buffer, n, bufsize);

    return bufsize;
}

int gen_random_text(unsigned char *buffer, int sigma, int bufsize)
{
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

double compute_average(double *T, int n)
{
    double avg = 0.0;
    for (int i = 0; i < n; i++)
        avg += T[i];

    return avg / n;
}

double compute_std(double avg, double *T, int n)
{
    double std = 0.0;
    for (int i = 0; i < n; i++)
        std += pow(avg - T[i], 2.0);

    return sqrt(std / n);
}

#define SEARCH_FUNC_NAME "search"

// TODO: dlclose() all lib_handle pointers
int load_algos(const char algo_names[][STR_BUF], int num_algos, int (**functions)(unsigned char *, int, unsigned char *, int))
{
    for (int i = 0; i < num_algos; i++)
    {
        char algo_lib_filename[STR_BUF];
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

void run_algo(const unsigned char **pattern_list, int m,
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

void print_benchmark_res(const char *output_line, run_command_opts_t *opts, benchmark_res_t *res)
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

int run_setting(unsigned char *T, int n, const run_command_opts_t *opts,
                char *code, char *time_format)
{
    printf("\tExperimental tests started on %s\n", time_format);

    unsigned char *pattern_list[opts->num_runs];
    for (int i = 0; i < opts->num_runs; i++) // TODO: allocate the correct size for each pattern
        pattern_list[i] = (unsigned char *)malloc(sizeof(unsigned char) * (PATTERN_SIZE_MAX + 1));

    unsigned char c;

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

void gen_experiment_code(char *code)
{
    sprintf(code, "EXP%d", (int)time(NULL));
}

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

void compute_frequency(const unsigned char *T, int n, int *freq)
{
    int nalpha = 0;
    int maxcode = 0;
    for (int j = 0; j < SIGMA; j++)
        freq[j] = 0;

    for (int j = 0; j < n; j++)
        freq[T[j]]++;
}

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

#define SMART_DATA_PATH_DEFAULT "data"
#define SMART_DATA_DIR_ENV "SMART_DATA_DIR"

const char *get_smart_data_dir()
{
    const char *path = getenv(SMART_DATA_DIR_ENV);
    return path != NULL ? path : SMART_DATA_PATH_DEFAULT;
}

int get_text(run_command_opts_t *opts, const char *filename, unsigned char *T)
{
    int size;
    if (!strcmp(filename, SMART_RANDOM_TEXT))
    {
        printf("\n\tTry to process random text with alphabet size of %d\n", opts->alphabet_size);
        size = gen_random_text(T, opts->alphabet_size, opts->text_size);
    }
    else
    {
        char fullpath[100];
        const char *data_path = get_smart_data_dir();
        sprintf(fullpath, "%s/%s", data_path, filename);
        printf("\n\tTry to process archive %s\n", fullpath);

        size = gen_search_text(fullpath, T, opts->text_size);
        if (size <= 0)
        {
            printf("\tunable to generate search text\n");
            exit(1);
        }
    }
    return size;
}

void run_benchmarks(run_command_opts_t *opts, unsigned char *T)
{

    char filename_list[NumSetting][50];
    int num_buffers = split_filename(opts->filename, filename_list);

    char expcode[STR_BUF];
    gen_experiment_code(expcode);

    printf("\tStarting experimental tests with code %s\n", expcode);

    for (int k = 0; k < num_buffers; k++)
    {
        int n = get_text(opts, filename_list[k], T);
        print_text_info(T, n);

        time_t date_timer;
        char time_format[26];
        struct tm *tm_info;
        time(&date_timer);
        tm_info = localtime(&date_timer);
        strftime(time_format, 26, "%Y:%m:%d %H:%M:%S", tm_info);

        run_setting(T, n, opts, expcode, time_format);
    }

    // outputINDEX(filename_list, num_buffers, expcode);
}

/*
 * Sets the scheduler affinity to pin this process to one CPU core.
 * This can avoid benchmarking variation caused by processes moving from one core to another, causing cache misses.
 */
void pin_to_one_CPU_core()
{
    cpu_set_t cpus;
    int last_cpu = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    int pid = getpid();
    CPU_ZERO(&cpus);
    CPU_SET(last_cpu, &cpus);
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpus) == -1)
    {
        printf("\n\tCould not pin the benchmark to a core - variation in benchmarking may be higher.\n\n");
    }
    else
    {
        printf("\n\tPinned benchmark process %d to core %d of %d processors.\n\n", pid, last_cpu, last_cpu + 1);
    }
}

int exec_run(run_command_opts_t *opts)
{
    print_logo();

    srand(opts->random_seed);
    printf("\n\tSetting random seed for this benchmarking run to %d\n", opts->random_seed);

    pin_to_one_CPU_core();

    unsigned char *T = (unsigned char *)malloc(sizeof(unsigned char) * (opts->text_size + PATTERN_SIZE_MAX));

    run_benchmarks(opts, T);

    free(T);

    return 0;
}