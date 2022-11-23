#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"
#include "config.h"

#define RUN_COMMAND "run"
#define SELECT_COMMAND "select"
#define MAX_DATA_SOURCES 20
#define NUM_RUNS_DEFAULT 500
#define ALPHABET_SIZE_DEFAULT 256
#define TIME_LIMIT_MILLIS_DEFAULT 300
#define CPU_PIN_DEFAULT "last"

#define PATTERN_MIN_LEN_DEFAULT 2
#define PATTERN_MAX_LEN_DEFAULT 4096
#define TEXT_SIZE_DEFAULT 1048576
#define MEGA_BYTE (1024 * 1024) // constant for 1 MB size
#define MAX_SELECT_ALGOS 2048

static char *const COMMAND_RUN = "run";
static char *const COMMAND_TEST = "test";
static char *const COMMAND_SELECT = "select";

static char *const OPTION_NUM_RUNS_SHORT = "-ps";
static char *const OPTION_NUM_RUNS_LONG = "--pset";
static char *const OPTION_TEXT_SIZE_SHORT = "-ts";
static char *const OPTION_TEXT_SIZE_LONG = "--tsize";
static char *const OPTION_MAX_TIME_SHORT = "-tb";
static char *const OPTION_MAX_TIME_LONG = "--time-bound";
static char *const OPTION_TEXT_SOURCE_SHORT = "-t";
static char *const OPTION_TEXT_SOURCE_LONG = "--text";
static char *const OPTION_RANDOM_TEXT_SHORT = "-rt";
static char *const OPTION_RANDOM_TEXT_LONG = "--rand-text";
static char *const OPTION_PATTERN_LEN_SHORT = "-pl";
static char *const OPTION_PATTERN_LEN_LONG = "--plen";
static char *const OPTION_SEED_SHORT = "-rs";
static char *const OPTION_SEED_LONG = "--rand-seed";
static char *const OPTION_SIMPLE_SHORT = "-s";
static char *const OPTION_SIMPLE_LONG = "--simple";
static char *const OPTION_CPU_PIN_SHORT = "-pin";
static char *const OPTION_CPU_PIN_LONG = "--pin-cpu";

static char *const ERROR_PARAMS_NOT_PROVIDED = "required parameters were not provided for option %s.";
static char *const ERROR_PARAM_NOT_INTEGER = "parameter for option %s is not an integer: %s";
static char *const ERROR_MUTUALLY_EXCLUSIVE = "mutually exclusive options you cannot have both %s and %s.";
static char *const ERROR_INTEGER_NOT_IN_RANGE = "parameter for option % must be between %d and %d";
static char *const ERROR_MAX_LESS_THAN_MIN = "max parameter %d for option %s must not be less than minimum %d";
static char *const ERROR_PARAM_TOO_BIG = "parameter for option %s is bigger than maximum length %d";
static char *const ERROR_UNKNOWN_PARAMETER = "unknown parameter %s provided for command %s";

static char *const FLAG_OCCURRENCE = "-occ";
static char *const FLAG_PREPROCESSING_TIME_SHORT = "-pre";
static char *const FLAG_PREPROCESSING_TIME_LONG = "--pre-time";
static char *const FLAG_TEXT_OUTPUT = "-txt";
static char *const FLAG_LATEX_OUTPUT = "-tex";
static char *const FLAG_PHP_OUTPUT = "-php";
static char *const FLAG_FILL_BUFFER_SHORT = "-fb";
static char *const FLAG_FILL_BUFFER_LONG = "--fill-buffer";
static char *const FLAG_SHORT_PATTERN_LENGTHS_SHORT = "-sh";
static char *const FLAG_SHORT_PATTERN_LENGTHS_LONG = "--short";
static char *const FLAG_VERY_SHORT_PATTERN_LENGTHS_SHORT = "-vs";
static char *const FLAG_VERY_SHORT_PATTERN_LENGTHS_LONG = "--vshort";

static char *const OPTION_SHORT_ADD = "-a";
static char *const OPTION_LONG_ADD = "--add";
static char *const OPTION_SHORT_REMOVE = "-r";
static char *const OPTION_LONG_REMOVE = "--remove";
static char *const OPTION_SHORT_SHOW_ALL = "-sa";
static char *const OPTION_LONG_SHOW_ALL = "--show-all";
static char *const OPTION_SHORT_SHOW_SELECTED = "-ss";
static char *const OPTION_LONG_SHOW_SELECTED = "--show-selected";
static char *const OPTION_SHORT_NO_ALGOS = "-n";
static char *const OPTION_LONG_NO_ALGOS = "--none";
static char *const OPTION_SHORT_LOAD = "-l";
static char *const OPTION_LONG_LOAD = "--load";
static char *const OPTION_SHORT_SAVE = "-s";
static char *const OPTION_LONG_SAVE = "--save";
static char *const OPTION_SHORT_LIST_SAVE = "-ls";
static char *const OPTION_LONG_LIST_SAVE = "--list-saved";

static char *const OPTION_SHORT_HELP = "-h";
static char *const OPTION_LONG_HELP = "--help";

typedef struct smart_opts
{
    const char *subcommand;
    const smart_config_t *smart_config;
    void *opts;
} smart_subcommand_t;

enum select_command_type
{
    ADD,
    REMOVE,
    SHOW_SELECTED,
    SHOW_ALL,
    LOAD,
    SAVE,
    LIST_SAVE,
    DESELECT_ALL,
    NO_COMMAND
};

typedef struct select_command_opts
{
    enum select_command_type select_command;
    const char *algos[MAX_SELECT_ALGOS];
    int n_algos;
} select_command_opts_t;

enum data_source_type
{
    NOT_DEFINED,
    FILES,
    RANDOM
};

typedef struct run_command_opts
{
    enum data_source_type data_source;          // What type of data is to be scanned - files or random.
    const char *data_sources[MAX_DATA_SOURCES]; // A list of files/data_sources to load data from.
    int text_size;                              // Size of the text buffer to fill for benchmarking.
    int alphabet_size;                          // Size of the alphabet to use when creating random texts.
    int pattern_min_len, pattern_max_len;
    int num_runs; // Number of patterns of a given length to benchmark.
    int time_limit_millis;
    long random_seed;        // Random seed used to generate text or patterns.
    const char *cpu_pinning; // Either auto, off or a number indicating the core to pin to.
    // flags

    int simple,
        fill_buffer, // whether to replicate data to fill up a buffer.
        occ,
        txt,
        pre,
        tex,
        php;
} run_command_opts_t;

void print_logo()
{
    printf("                                \n");
    printf("	                          _   \n");
    printf("	 ___ _ __ ___   __ _ _ __| |_ \n");
    printf("	/ __|  _   _ \\ / _  |  __| __|\n");
    printf("	\\__ \\ | | | | | (_| | |  | |_ \n");
    printf("	|___/_| |_| |_|\\__,_|_|   \\__|\n");
    printf("	A String Matching Research Tool\n");
    printf("	by Math Palmer, Simone Faro, Stefano Scafiti and Thierry Lecroq\n");
    printf("	Last Update: May 2017\n");
    printf("\n");
    printf("	If you use this tool in your research please cite the following paper:\n");
    printf("	| Simone Faro and Thierry Lecroq\n");
    printf("	| The Exact Online String Matching Problem: a Review of the Most Recent Results\n");
    printf("	| ACM Computing Surveys, Vol. 45(2): p.13 (2013)\n");
    printf(" ");
}

void print_subcommand_usage_and_exit(const char *command)
{
    print_logo();

    printf("\n usage: %s [run | test | select]\n\n", command);

    printf("\t- run: executes benchmarks on one or more algorithms\n");
    printf("\t- test: test the correctness of an algorithm\n");
    printf("\t- select: select one or more algorithms to run\n");
    printf("\n\n");

    exit(0);
}

void print_help_line(const char *description, const char *short_option, const char *long_option, const char *params)
{
    printf("\t%-4s %-16s %-8s %s\n", short_option, long_option, params, description);
}

void print_run_usage_and_exit(const char *command)
{
    print_logo();

    printf("\n\tThis is a basic help guide for using the tool:\n\n");
    print_help_line("performs experimental results loading all files F specified into a single buffer for benchmarking.", OPTION_TEXT_SOURCE_SHORT, OPTION_TEXT_SOURCE_LONG, "F ...");
    print_help_line("You can specify several individual files, or directories.  If a directory, all files in it will be loaded,", "", "", "");
    print_help_line("up to the maximum buffer size.  SMART will look for files locally, and then in its search", "", "", "");
    print_help_line("path, which defaults to the /data directory in the smart distribution.", "", "", "");
    print_help_line("performs experimental results using random text with an alphabet A between 1 and 256 inclusive", OPTION_RANDOM_TEXT_SHORT, OPTION_RANDOM_TEXT_LONG, "A");
    print_help_line("You can only specify a text or a random text - not both.", "", "", "");
    print_help_line("computes running times as the mean of N runs (default 500)", OPTION_NUM_RUNS_SHORT, OPTION_NUM_RUNS_LONG, "N");
    print_help_line("set the upper bound dimension (in Mb) of the text used for experimental results (default 1Mb)", OPTION_TEXT_SIZE_SHORT, OPTION_TEXT_SIZE_LONG, "S");
    print_help_line("fills the text buffer up to its maximum size by copying earlier data until full.", FLAG_FILL_BUFFER_SHORT, FLAG_FILL_BUFFER_LONG, "");
    print_help_line("test only patterns with a length between L and U (included).", OPTION_PATTERN_LEN_SHORT, OPTION_PATTERN_LEN_LONG, "L U");
    print_help_line("computes experimental results using short length patterns (from 2 to 32)", FLAG_SHORT_PATTERN_LENGTHS_SHORT, FLAG_SHORT_PATTERN_LENGTHS_LONG, "");
    print_help_line("computes experimental results using very short length patterns (from 1 to 16)", FLAG_VERY_SHORT_PATTERN_LENGTHS_SHORT, FLAG_VERY_SHORT_PATTERN_LENGTHS_LONG, "");
    print_help_line("sets the random seed to integer S, ensuring tests and benchmarks can be precisely repeated.", OPTION_SEED_SHORT, OPTION_SEED_LONG, "S");
    print_help_line("computes separately preprocessing times and searching times", FLAG_PREPROCESSING_TIME_SHORT, FLAG_PREPROCESSING_TIME_LONG, "");
    print_help_line("set to L the upper bound for any worst case running time (in ms). The default value is 300 ms", OPTION_MAX_TIME_SHORT, OPTION_MAX_TIME_LONG, "L");
    print_help_line("sets whether cpu pinning is off, set to the last cpu core, or a specific core via parameter C:  off | last | {digit}", OPTION_CPU_PIN_SHORT, OPTION_CPU_PIN_LONG, "C");
    print_help_line("prints the total number of occurrences", FLAG_OCCURRENCE, "", "");
    print_help_line("output results in txt tabular format", FLAG_TEXT_OUTPUT, "", "");
    print_help_line("output results in latex tabular format", FLAG_LATEX_OUTPUT, "", "");
    print_help_line("executes a single run searching T (max 1000 chars) for occurrences of P (max 100 chars)", OPTION_SIMPLE_SHORT, OPTION_SIMPLE_LONG, "P T");
    print_help_line("gives this help list", OPTION_SHORT_HELP, OPTION_LONG_HELP, "");
    printf("\n\n");

    exit(0);
}

void print_select_usage_and_exit(const char *command)
{
    print_logo();

    printf("\n usage: %s select [algo1, algo2, ...] [ -a | -r | -sa | -ss | -n | -l | -s | -ls | -h ]\n\n", command);

    print_help_line("add the list of specified algorithms to the set.  Algorithm names are specified as POSIX extended regular expressions.", OPTION_SHORT_ADD, OPTION_LONG_ADD, "algo...");
    print_help_line("remove the list of specified algorithms to the set.  Algorithm names are specified as POSIX extended regular expressions.", OPTION_SHORT_REMOVE, OPTION_LONG_REMOVE, "algo...");
    print_help_line("clears all selected algorithms", OPTION_SHORT_NO_ALGOS, OPTION_LONG_NO_ALGOS, "");
    print_help_line("saves the current selected algorithm set with name N.  This copies the selected_algos file to a new file N.algos.", OPTION_SHORT_SAVE, OPTION_LONG_SAVE, "N");
    print_help_line("loads a previously saved set of algorithms with name N.  This overwrites the selected_algos file with the contents of N.algos.", OPTION_SHORT_LOAD, OPTION_LONG_LOAD, "N");
    print_help_line("lists previously saved selected algorithm sets in the config folder.", OPTION_SHORT_LIST_SAVE, OPTION_LONG_LIST_SAVE, "");
    print_help_line("shows the list of all algorithms.", OPTION_SHORT_SHOW_ALL, OPTION_LONG_SHOW_ALL, "");
    print_help_line("shows the list of all selected algorithms.", OPTION_SHORT_SHOW_SELECTED, OPTION_LONG_SHOW_SELECTED, "");
    print_help_line("gives this help list.", OPTION_SHORT_HELP, OPTION_LONG_HELP, "");
    printf("\n\n");

    exit(0);
}

void print_test_usage_and_exit()
{
    print_logo();

    printf("\n\tSMART UTILITY FOR TESTING STRING MATCHING ALGORITHMS\n\n");
    printf("\tusage: ./test ALGONAME\n");
    printf("\tTest the program named \"algoname\" for correctness.\n");
    printf("\tThe program \"algoname\" must be located in source/bin/\n");
    printf("\tOnly programs in smart format can be tested.\n");
    printf("\n\n");
}

void print_error_message_and_exit(const char *message)
{
    printf("Error in input parameters: %s\nUse -h for help.\n\n", message);
    exit(1);
}

void print_format_error_message_and_exit(const char *format, ...)
{
    printf("Error in input parameters: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\nUse -h for help.\n\n");
    exit(1);
}

void opts_init_default(run_command_opts_t *opts)
{
    opts->data_source = NOT_DEFINED;
    for (int i = 0; i < MAX_DATA_SOURCES; i++)
        opts->data_sources[i] = NULL;
    opts->cpu_pinning = CPU_PIN_DEFAULT;
    opts->alphabet_size = ALPHABET_SIZE_DEFAULT;
    opts->text_size = TEXT_SIZE_DEFAULT;
    opts->pattern_min_len = PATTERN_MIN_LEN_DEFAULT;
    opts->pattern_max_len = PATTERN_MAX_LEN_DEFAULT;
    opts->num_runs = NUM_RUNS_DEFAULT;
    opts->time_limit_millis = TIME_LIMIT_MILLIS_DEFAULT;
    opts->random_seed = time(NULL);
    opts->simple = 0;
    opts->fill_buffer = 0;
    opts->pre = 0;
    opts->occ = 0;
    opts->txt = 0;
    opts->tex = 0;
    opts->php = 0;
}

void check_has_params(int curr_arg, int argc, const char *option)
{
    if (curr_arg >= argc)
        print_format_error_message_and_exit(ERROR_PARAMS_NOT_PROVIDED, option);
}

void check_is_int(const char *param, const char *option)
{
    if (!is_int(param))
        print_format_error_message_and_exit(ERROR_PARAM_NOT_INTEGER, option, param);
}

void check_string_length(const char *param, int maxlen, const char *option)
{
    if (strlen(param) > maxlen)
        print_format_error_message_and_exit(ERROR_PARAM_TOO_BIG, option, maxlen);
}

int parse_num_runs(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_NUM_RUNS_LONG);
    check_is_int(argv[curr_arg + 1], OPTION_NUM_RUNS_LONG);

    line->num_runs = atoi(argv[curr_arg + 1]);
    return 1;
}

int parse_text_size(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_TEXT_SIZE_LONG);
    check_is_int(argv[curr_arg + 1], OPTION_TEXT_SIZE_LONG);

    line->text_size = atoi(argv[curr_arg + 1]) * MEGA_BYTE;
    return 1;
}

int parse_time_limit(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_MAX_TIME_LONG);
    check_is_int(argv[curr_arg + 1], OPTION_MAX_TIME_LONG);

    line->time_limit_millis = atoi(argv[curr_arg + 1]);
    return 1;
}

int parse_text(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_TEXT_SOURCE_LONG);

    if (line->data_source == RANDOM)
        print_format_error_message_and_exit(ERROR_MUTUALLY_EXCLUSIVE, OPTION_TEXT_SOURCE_LONG, OPTION_RANDOM_TEXT_LONG);

    int name_arg = 0;
    while (name_arg < MAX_DATA_SOURCES && curr_arg + name_arg + 1 < argc && argv[curr_arg + name_arg + 1][0] != '-')
    {
        line->data_sources[name_arg] = argv[curr_arg + name_arg + 1];
        name_arg++;
    }

    // TODO: test that there are not more parameters after MAX_DATA_SOURCES?

    if (name_arg > 0)
    {
        line->data_source = FILES;
    }
    else
    {
        print_format_error_message_and_exit(ERROR_PARAMS_NOT_PROVIDED, OPTION_TEXT_SOURCE_LONG);
    }

    return name_arg;
}

int parse_random_text(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_RANDOM_TEXT_LONG);
    check_is_int(argv[curr_arg + 1], OPTION_RANDOM_TEXT_LONG);

    if (line->data_source == FILES)
        print_format_error_message_and_exit(ERROR_MUTUALLY_EXCLUSIVE, OPTION_RANDOM_TEXT_LONG, OPTION_TEXT_SOURCE_LONG);

    line->alphabet_size = atoi(argv[curr_arg + 1]);
    line->data_source = RANDOM;

    if (line->alphabet_size < 1 || line->alphabet_size > 256)
        print_format_error_message_and_exit(ERROR_INTEGER_NOT_IN_RANGE, OPTION_RANDOM_TEXT_LONG, 1, 256);

    return 1;
}

int parse_pattern_len(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_PATTERN_LEN_LONG);
    check_is_int(argv[curr_arg + 1], OPTION_PATTERN_LEN_LONG);

    line->pattern_min_len = atoi(argv[curr_arg + 1]);

    check_has_params(curr_arg + 2, argc, OPTION_PATTERN_LEN_LONG);
    check_is_int(argv[curr_arg + 2], OPTION_PATTERN_LEN_LONG);

    line->pattern_max_len = atoi(argv[curr_arg + 2]);

    if (line->pattern_max_len < line->pattern_min_len)
        print_format_error_message_and_exit(ERROR_MAX_LESS_THAN_MIN, line->pattern_max_len, OPTION_PATTERN_LEN_LONG, line->pattern_min_len);

    return 2;
}

int parse_seed(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_SEED_LONG);
    check_is_int(argv[curr_arg + 1], OPTION_SEED_LONG);

    line->random_seed = atol(argv[curr_arg + 1]);

    return 1;
}

int parse_cpu_pinning(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_CPU_PIN_LONG);

    char param[STR_BUF];
    strcpy(param, argv[curr_arg + 1]);
    str2lower(param);
    if (!strcmp(param, "off") || !strcmp(param, "last") || is_int(param))
    {
        line->cpu_pinning = param;
    }
    else
    {
        print_format_error_message_and_exit("Incorrect parameter %s for option %s.  Must be off | last | {digit}", argv[curr_arg + 1], OPTION_CPU_PIN_LONG);
    }

    return 1;
}

int parse_simple(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    check_has_params(curr_arg + 1, argc, OPTION_SIMPLE_LONG);
    check_string_length(argv[curr_arg + 1], 100, OPTION_SIMPLE_LONG);

    check_has_params(curr_arg + 2, argc, OPTION_SIMPLE_LONG);
    check_string_length(argv[curr_arg + 2], 1000, OPTION_SIMPLE_LONG);

    // TODO: set simple search params somewhere! This only checks they are correct - doesn't record them.

    line->simple = 1;
    return 2;
}

int parse_flag(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    if (!strcmp(FLAG_OCCURRENCE, argv[curr_arg]))
    {
        line->occ = 1;
        return 1;
    }
    else if (!strcmpany(argv[curr_arg], 2, FLAG_PREPROCESSING_TIME_SHORT, FLAG_PREPROCESSING_TIME_LONG))
    {
        line->pre = 1;
        return 1;
    }
    if (!strcmp(FLAG_TEXT_OUTPUT, argv[curr_arg]))
    {
        line->txt = 1;
        return 1;
    }
    if (!strcmp(FLAG_LATEX_OUTPUT, argv[curr_arg]))
    {
        line->tex = 1;
        return 1;
    }
    if (!strcmp(FLAG_PHP_OUTPUT, argv[curr_arg]))
    {
        line->php = 1;
        return 1;
    }
    if (!strcmpany(argv[curr_arg], 2, FLAG_SHORT_PATTERN_LENGTHS_SHORT, FLAG_SHORT_PATTERN_LENGTHS_LONG))
    {
        // TODO: PATT_SIZE = PATT_SHORT_SIZE;
        return 1;
    }
    if (!strcmpany(argv[curr_arg], 2, FLAG_VERY_SHORT_PATTERN_LENGTHS_SHORT, FLAG_VERY_SHORT_PATTERN_LENGTHS_LONG))
    {
        // TODO: PATT_SIZE = PATT_VERY_SHORT;
        return 1;
    }
    if (!strcmpany(argv[curr_arg], 2, FLAG_FILL_BUFFER_SHORT, FLAG_FILL_BUFFER_LONG))
    {
        line->fill_buffer = 1;
        return 1;
    }
    return 0;
}

void parse_run_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    run_command_opts_t *opts = malloc(sizeof(run_command_opts_t));
    subcommand->opts = opts;

    if (argc <= 3)
        print_run_usage_and_exit(argv[1]);

    opts_init_default(opts);

    for (int curr_arg = 2; curr_arg < argc; curr_arg++)
    {
        if (!strcmpany(argv[curr_arg], 2, OPTION_NUM_RUNS_SHORT, OPTION_NUM_RUNS_LONG))
        {
            curr_arg += parse_num_runs(opts, curr_arg, argc, argv);
        }
        else if (!strcmpany(argv[curr_arg], 2, OPTION_TEXT_SIZE_SHORT, OPTION_TEXT_SIZE_LONG))
        {
            curr_arg += parse_text_size(opts, curr_arg, argc, argv);
        }
        else if (!strcmpany(argv[curr_arg], 2, OPTION_MAX_TIME_SHORT, OPTION_MAX_TIME_LONG))
        {
            curr_arg += parse_time_limit(opts, curr_arg, argc, argv);
        }
        else if (!strcmpany(argv[curr_arg], 2, OPTION_TEXT_SOURCE_SHORT, OPTION_TEXT_SOURCE_LONG))
        {
            curr_arg += parse_text(opts, curr_arg, argc, argv);
        }
        else if (!strcmpany(argv[curr_arg], 2, OPTION_RANDOM_TEXT_SHORT, OPTION_RANDOM_TEXT_LONG))
        {
            curr_arg += parse_random_text(opts, curr_arg, argc, argv);
        }
        else if (!strcmpany(argv[curr_arg], 2, OPTION_PATTERN_LEN_SHORT, OPTION_PATTERN_LEN_LONG))
        {
            curr_arg += parse_pattern_len(opts, curr_arg, argc, argv);
        }
        else if (!strcmpany(argv[curr_arg], 2, OPTION_SEED_SHORT, OPTION_SEED_LONG))
        {
            curr_arg += parse_seed(opts, curr_arg, argc, argv);
        }
        else if (!strcmpany(argv[curr_arg], 2, OPTION_SIMPLE_SHORT, OPTION_SIMPLE_LONG))
        {
            curr_arg += parse_simple(opts, curr_arg, argc, argv);
        }
        else if (!strcmpany(argv[curr_arg], 2, OPTION_CPU_PIN_SHORT, OPTION_CPU_PIN_LONG))
        {
            curr_arg += parse_cpu_pinning(opts, curr_arg, argc, argv);
        }
        else if (!parse_flag(opts, curr_arg, argc, argv))
        {
            print_format_error_message_and_exit(ERROR_UNKNOWN_PARAMETER, argv[curr_arg], COMMAND_RUN);
        }
    }

    if (opts->data_source == NOT_DEFINED)
        print_error_message_and_exit("No data source is defined with -text or -random.");
}

void parse_test_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    print_test_usage_and_exit();
    // TODO: parse test.
}

int is_flag_argument(const char *arg)
{
    return strncmp("-", arg, 1) == 0;
}

int select_command_has_params(enum select_command_type select_command)
{
    return select_command == ADD || select_command == REMOVE || select_command == LOAD || select_command == SAVE;
}

void parse_select_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    select_command_opts_t *opts = malloc(sizeof(select_command_opts_t));
    opts->select_command = NO_COMMAND;

    if (argc <= 2)
        print_select_usage_and_exit(argv[0]);

    int k = 0;
    for (int i = 2; i < argc; i++)
    {
        if (!strcmpany(argv[i], 2, OPTION_SHORT_ADD, OPTION_LONG_ADD))
        {
            opts->select_command = ADD;
        }
        else if (!strcmpany(argv[i], 2, OPTION_SHORT_REMOVE, OPTION_LONG_REMOVE))
        {
            opts->select_command = REMOVE;
        }
        else if (!strcmpany(argv[i], 2, OPTION_SHORT_SHOW_ALL, OPTION_LONG_SHOW_ALL))
        {
            opts->select_command = SHOW_ALL;
        }
        else if (!strcmpany(argv[i], 2, OPTION_SHORT_SHOW_SELECTED, OPTION_LONG_SHOW_SELECTED))
        {
            opts->select_command = SHOW_SELECTED;
        }
        else if (!strcmpany(argv[i], 2, OPTION_SHORT_NO_ALGOS, OPTION_LONG_NO_ALGOS))
        {
            opts->select_command = DESELECT_ALL;
        }
        else if (!strcmpany(argv[i], 2, OPTION_SHORT_LOAD, OPTION_LONG_LOAD))
        {
            opts->select_command = LOAD;
        }
        else if (!strcmpany(argv[i], 2, OPTION_SHORT_SAVE, OPTION_LONG_SAVE))
        {
            opts->select_command = SAVE;
        }
        else if (!strcmpany(argv[i], 2, OPTION_SHORT_LIST_SAVE, OPTION_LONG_LIST_SAVE))
        {
            opts->select_command = LIST_SAVE;
        }
        else if (!strcmpany(argv[i], 2, OPTION_SHORT_HELP, OPTION_LONG_HELP))
        {
            print_select_usage_and_exit(argv[0]);
        }
        else if (is_flag_argument(argv[i]))
        {
            print_format_error_message_and_exit("%s: unrecognized option \"%s\"\n", argv[1], argv[i]);
        }
        else
        {
            opts->algos[k++] = argv[i];
        }
    }

    if (k == 0 && select_command_has_params(opts->select_command))
        print_select_usage_and_exit(argv[0]);

    if (k > 0 && !(select_command_has_params(opts->select_command)))
        print_select_usage_and_exit(argv[0]);

    if ((opts->select_command == LOAD || opts->select_command == SAVE) && k != 1)
        print_select_usage_and_exit(argv[0]);

    opts->n_algos = k;

    subcommand->opts = (void *)opts;
}

void parse_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    if (argc <= 1 || !strcmpany(argv[1], 2, OPTION_SHORT_HELP, OPTION_LONG_HELP))
        print_subcommand_usage_and_exit(argv[0]);

    subcommand->subcommand = argv[1];

    if (!strcmp(argv[1], COMMAND_RUN))
    {
        parse_run_args(argc, argv, subcommand);
    }
    else if (!strcmp(argv[1], COMMAND_TEST))
    {
        parse_test_args(argc, argv, subcommand);
    }
    else if (!strcmp(argv[1], COMMAND_SELECT))
    {
        parse_select_args(argc, argv, subcommand);
    }
    else
    {
        print_subcommand_usage_and_exit(argv[0]);
    }
}

#endif