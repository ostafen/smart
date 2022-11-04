#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "utils.h"

#define RUN_COMMAND "run"
#define SELECT_COMMAND "select"

typedef struct smart_opts
{
    const char *subcommand;
    void *opts;
} smart_subcommand_t;

#define MAX_SELECT_ALGOS 100

typedef struct select_command_opts
{
    int add;
    int show_selected;
    int show_all;
    int deselect_all;
    const char *algos[MAX_SELECT_ALGOS];
    int n_algos;
} select_command_opts_t;

typedef struct run_command_opts
{
    const char *filename;
    int text_size, alphabet_size;
    int pattern_min_len, pattern_max_len;
    int num_runs;
    int time_limit_millis;

    // flags

    int simple,
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
    printf("	by Simone Faro, Stefano Scafiti and Thierry Lecroq\n");
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
    printf("usage: %s [run | test | select | textgen]\n\n", command);
    printf("\t- run: executes benchmarks on one or more algorithms\n\n");
    printf("\t- test: test the correctness of an algorithm\n\n");
    printf("\t- select: select one or more algorithms to run\n\n");
    printf("\t- textgen: random text buffers generation utility\n");
    printf("\n\n");

    exit(0);
}

void print_run_usage_and_exit(const char *command)
{
    print_logo();

    printf("\tThis is a basic help guide for using the tool\n\n");
    printf("\t-pset N       computes running times as the mean of N runs (default 500)\n");
    printf("\t-tsize S      set the upper bound dimension (in Mb) of the text used for experimental results (default 1Mb)\n");
    printf("\t-plen L U     test only patterns with a length between L and U (included).\n");
    printf("\t-text F       performs experimental results using text buffer F (mandatory unless you use the -simple parameter)\n");
    printf("\t              Use option \"all\" to performe experimental results using all text buffers.\n");
    printf("\t              Use the style A-B-C to performe experimental results using multiple text buffers.\n");
    printf("\t              Separate the list of text buffers using the symbol \"-\"\n");
    printf("\t-short        computes experimental results using short length patterns (from 2 to 32)\n");
    printf("\t-vshort       computes experimental results using very short length patterns (from 1 to 16)\n");
    printf("\t-occ          prints the average number of occurrences\n");
    printf("\t-pre          computes separately preprocessing times and searching times\n");
    printf("\t-tb L         set to L the upper bound for any wort case running time (in ms). The default value is 300 ms\n");
    printf("\t-txt          output results in txt tabular format\n");
    printf("\t-tex          output results in latex tabular format\n");
    printf("\t-simple P T   executes a single run searching T (max 1000 chars) for occurrences of P (max 100 chars)\n");
    printf("\t-h            gives this help list\n");
    printf("\n\n");

    exit(0);
}

#define NUM_RUNS_DEFAULT 500
#define ALPHABET_SIZE_DEFAULT 256
#define TIME_LIMIT_MILLIS_DEFAULT 300

#define PATTERN_MIN_LEN_DEFAULT 2
#define PATTERN_MAX_LEN_DEFAULT 4096
#define TEXT_SIZE_DEFAULT 1048576

void opts_init_default(run_command_opts_t *opts)
{
    opts->filename = NULL;
    opts->text_size = -1;
    opts->alphabet_size = ALPHABET_SIZE_DEFAULT;
    opts->text_size = TEXT_SIZE_DEFAULT;
    opts->pattern_min_len = PATTERN_MIN_LEN_DEFAULT;
    opts->pattern_max_len = PATTERN_MAX_LEN_DEFAULT;
    opts->num_runs = NUM_RUNS_DEFAULT;
    opts->alphabet_size = -1;
    opts->num_runs = NUM_RUNS_DEFAULT;
    opts->time_limit_millis = TIME_LIMIT_MILLIS_DEFAULT;
    opts->simple = 0;
    opts->occ = 0;
    opts->txt = 0;
    opts->tex = 0;
    opts->php = 0;
}

void print_error_and_exit()
{
    printf("Error in input parameters. Use -h for help.\n\n");
    exit(1);
}

int parse_num_runs(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    if (curr_arg + 1 >= argc)
        print_error_and_exit();

    const char *num_runs_str = argv[curr_arg + 1];
    if (!is_int(num_runs_str))
        print_error_and_exit();

    line->num_runs = atoi(num_runs_str);
    return 1;
}

#define MEGA_BYTE (1024 * 1024) // costant for 1 MB size

int parse_text_size(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    if (curr_arg + 1 >= argc)
        print_error_and_exit();

    const char *text_size_str = argv[curr_arg + 1];
    if (!is_int(text_size_str))
        print_error_and_exit();

    line->text_size = atoi(text_size_str) * MEGA_BYTE;
    return 1;
}

int parse_time_limit(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    if (curr_arg + 1 >= argc)
    {
        print_error_and_exit();
    }

    const char *time_limit_str = argv[curr_arg + 1];
    if (!is_int(time_limit_str))
    {
        print_error_and_exit();
    }
    line->time_limit_millis = atoi(time_limit_str);
    return 1;
}

int parse_text(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    if (curr_arg + 1 >= argc)
        print_error_and_exit();

    const char *filename = argv[curr_arg + 1];
    line->filename = filename;
    return 1;
}

int parse_pattern_len(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    if (curr_arg + 1 >= argc)
        print_error_and_exit();

    line->pattern_min_len = atoi(argv[curr_arg + 1]);

    if (curr_arg + 2 >= argc)
        print_error_and_exit();

    line->pattern_max_len = atoi(argv[curr_arg + 2]);
    if (line->pattern_max_len < line->pattern_min_len)
        print_error_and_exit();

    return 2;
}

int parse_simple(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    if (curr_arg + 1 >= argc)
        print_error_and_exit();

    const char *simple_pattern = argv[curr_arg + 1];
    if (strlen(simple_pattern) > 100)
        print_error_and_exit();

    const char *simple_text = argv[curr_arg + 2];

    if (strlen(simple_text) > 1000)
        print_error_and_exit();

    line->simple = 1;
    return 2;
}

int parse_flag(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    if (!strcmp("-occ", argv[curr_arg]))
    {
        line->occ = 1;
        return 1;
    }
    else if (!strcmp("-pre", argv[curr_arg]))
    {
        line->pre = 1;
        return 1;
    }
    if (!strcmp("-txt", argv[curr_arg]))
    {
        line->txt = 1;
        return 1;
    }
    if (!strcmp("-tex", argv[curr_arg]))
    {
        line->tex = 1;
        return 1;
    }
    if (!strcmp("-php", argv[curr_arg]))
    {
        line->php = 1;
        return 1;
    }
    if (!strcmp("-short", argv[curr_arg]))
    {
        // PATT_SIZE = PATT_SHORT_SIZE;
        return 1;
    }
    if (!strcmp("-vshort", argv[curr_arg]))
    {
        // PATT_SIZE = PATT_VERY_SHORT;
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
        if (!strcmp("-pset", argv[curr_arg]))
        {
            curr_arg += parse_num_runs(opts, curr_arg, argc, argv);
        }
        else if (!strcmp("-tsize", argv[curr_arg]))
        {
            curr_arg += parse_text_size(opts, curr_arg, argc, argv);
        }
        else if (!strcmp("-tb", argv[curr_arg]))
        {
            curr_arg += parse_time_limit(opts, curr_arg, argc, argv);
        }
        else if (!strcmp("-text", argv[curr_arg]))
        {
            curr_arg += parse_text(opts, curr_arg, argc, argv);
        }
        else if (!strcmp("-plen", argv[curr_arg]))
        {
            curr_arg += parse_pattern_len(opts, curr_arg, argc, argv);
        }
        else if (!strcmp("-simple", argv[curr_arg]))
        {
            curr_arg += parse_simple(opts, curr_arg, argc, argv);
        }
        else if (!parse_flag(opts, curr_arg, argc, argv))
        {
            print_error_and_exit();
        }
    }

    if (!strcmp(opts->filename, ""))
        print_error_and_exit();
}

void print_test_usage_and_exit()
{
    printf("\n\tSMART UTILITY FOR TESTING STRING MATCHING ALGORITHMS\n\n");
    printf("\tusage: ./test ALGONAME\n");
    printf("\tTest the program named \"algoname\" for correctness.\n");
    printf("\tThe program \"algoname\" must be located in source/bin/\n");
    printf("\tOnly programs in smart format can be tested.\n");
    printf("\n\n");
}

void parse_test_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    print_test_usage_and_exit();
}

void print_select_usage_and_exit(const char *command)
{
    printf("usage: %s select algo1, algo2, ... [-a | -r | -sa | -ss | -n | -h]\n\n", command);
    printf("\t-a, --add:            \tadd the list of specified algorithms to the set\n");
    printf("\t-r, --remove:         \tremove the list of specified algorithms to the set\n");
    printf("\t-sa, --show-all:      \tshows the list of all algorithms\n");
    printf("\t-ss, --show-selected: \tshows the list of all selected algorithms\n");
    printf("\t-h:                   \tgives this help list\n");
    printf("\n\n");

    exit(0);
}

void unrecognized_param_error(const char *command, const char *option)
{
    printf("%s: unrecognized option \"%s\"\n", command, option);
    exit(1);
}

int strcmpany(const char *s, int n, ...)
{
    va_list ptr;

    va_start(ptr, n);
    for (int i = 0; i < n; i++)
    {
        const char *curr_str = va_arg(ptr, const char *);
        if (!strcmp(curr_str, s))
            return 0;
    }
    va_end(ptr);

    return -1;
}

int is_flag_argument(const char *arg)
{
    return strncmp("-", arg, 1) == 0 || strncmp("--", arg, 2) == 0;
}

#define STR_BUFSIZE 50

void parse_select_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    select_command_opts_t *opts = malloc(sizeof(select_command_opts_t));
    opts->add = 1;
    opts->show_all = 0;
    opts->show_selected = 0;
    opts->deselect_all = 0;

    if (argc <= 2)
        print_select_usage_and_exit(argv[0]);

    int k = 0;
    for (int i = 2; i < argc; i++)
    {
        if (!strcmpany(argv[i], 2, "-a", "--add"))
        {
            opts->add = 1;
        }
        else if (!strcmpany(argv[i], 3, "-r", "-remove", "--remove"))
        {
            opts->add = 0;
        }
        else if (!strcmpany(argv[i], 3, "-sa", "-show-all", "--show-all"))
        {
            opts->show_all = 1;
        }
        else if (!strcmpany(argv[i], 3, "-ss", "-show-selected", "--show-selected"))
        {
            opts->show_selected = 1;
        }
        else if (!strcmpany(argv[i], 2, "-n", "--none"))
        {
            opts->deselect_all = 1;
        }
        else if (!strcmp(argv[i], "-h"))
        {
            print_select_usage_and_exit(argv[0]);
        }
        else if (is_flag_argument(argv[i]))
        {
            unrecognized_param_error(argv[1], argv[i]);
        }
        else
        {
            opts->algos[k++] = argv[i];
        }
    }

    if (k == 0 && (!opts->show_all && !opts->show_selected && !opts->deselect_all))
        print_select_usage_and_exit(argv[0]);

    if (k > 0 && (opts->show_all || opts->show_selected || opts->deselect_all))
        print_select_usage_and_exit(argv[0]);

    opts->n_algos = k;

    subcommand->opts = (void *)opts;
}

void parse_textgen_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    printf("textgen args\n");
}

void parse_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    if (argc <= 1 || !strcmp(argv[1], "-h"))
        print_subcommand_usage_and_exit(argv[0]);

    subcommand->subcommand = argv[1];

    if (!strcmp(argv[1], "run"))
    {
        parse_run_args(argc, argv, subcommand);
    }
    else if (!strcmp(argv[1], "test"))
    {
        parse_test_args(argc, argv, subcommand);
    }
    else if (!strcmp(argv[1], "select"))
    {
        parse_select_args(argc, argv, subcommand);
    }
    else if (!strcmp(argv[1], "textgen"))
    {
        parse_textgen_args(argc, argv, subcommand);
    }
    else
    {
        print_subcommand_usage_and_exit(argv[0]);
    }
}

#endif