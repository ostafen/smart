#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "defines.h"
#include "utils.h"
#include "config.h"
#include "commands.h"
#include "cpu_stats.h"

//TODO: standardise "usage" syntax for optional params, etc.

/*
 * Error messages.
 */
static char *const ERROR_PARAMS_NOT_PROVIDED = "%srequired parameters were not provided for option %s.%s";
static char *const ERROR_PARAM_NOT_INTEGER  = "%sparameter for option %s is not an integer: %s%s";
static char *const ERROR_MUTUALLY_EXCLUSIVE = "%smutually exclusive options you cannot have both %s and %s.%s";
static char *const ERROR_INTEGER_NOT_IN_RANGE = "%sparameter for option %s must be between %d and %d%s";
static char *const ERROR_INTEGER_TOO_SMALL = "%sparameter '%s' for option %s must be at least %d%s";
static char *const ERROR_MAX_LESS_THAN_MIN = "%smax parameter %d for option %s must not be less than minimum %d%s";
static char *const ERROR_PARAM_TOO_BIG = "%sparameter for option %s is bigger than maximum length %d%s";
static char *const ERROR_CPU_PINNING_PARAMETER = "%sIncorrect parameter %s for option %s.  Must be off | last | {digit}%s";
static char *const ERROR_MISSING_PARAMETER = "%soption %s needs a parameter, next is a -flag parameter: %s%s";
static char *const ERROR_NO_DATA_SOURCE_DEFINED = "%sno data source is defined with either %s or %s%s";
static char *const ERROR_TOO_MANY_DATA_SOURCES_DEFINED = "%sToo many data source are defined with either %s.  Max is %d.%s";
static char *const ERROR_UNRECOGNISED_OPTION = "%s%s: unrecognized option %s%s";
static char *const ERROR_MIN_PATTERN_BIGGER_THAN_TEXT = "%sMinimum pattern length %d is bigger than the text size of %d.%s";
static char *const ERROR_NO_ALGORITHMS_DEFINED = "%sno algorithms specified for test.%s";
static char *const ERROR_TOO_MANY_PATTERN_LENGTHS = "%sToo many patterns lengths specified: %d, from %d to %d, incrementing %c %d.  Max = %d%s";
static char *const ERROR_PARAM_TOO_SHORT = "%sParameter '%s' value for option %s must be at least %d long.%s";
static char *const ERROR_INCREMENT_PLUS_OR_TIMES = "%sIncrement option %s parameter '%s' must start with + or *%s";
static char *const ERROR_HEADER = "incorrect input parameters: ";
static char *const ERROR_FOOTER = "\n\nUse -h for help.\n\n";

/*
 * Exits if we have run out of parameters.
 */
void check_end_of_params(int curr_arg, int argc, const char *option)
{
    if (curr_arg >= argc)
        error_and_exit(ERROR_PARAMS_NOT_PROVIDED, ERROR_HEADER, option, ERROR_FOOTER);
}

/*
 * Exits if the value is not an int.
 */
void check_is_int(const char *param, const char *option)
{
    if (!is_int(param))
        error_and_exit(ERROR_PARAM_NOT_INTEGER, ERROR_HEADER, option, param, ERROR_FOOTER);
}

/*
 * Returns true if the argument is prefixed with a -, meaning it is a command option.
 */
int is_command_option(const char *arg)
{
    return arg[0] == '-';
}

/*
 * Returns true if there is another parameter following the current one that is not a command option.
 */
int next_is_param(const int curr_arg, const int argc, const char **argv)
{
    return curr_arg + 1 < argc && !is_command_option(argv[curr_arg + 1]);
}

/*
 * Exits if the option is a flag parameter (-something) - this means we have a missing parameter for the current flag command.
 */
void check_is_not_a_command_option(const char *arg, const char *option)
{
    if (is_command_option(arg))
    {
        error_and_exit(ERROR_MISSING_PARAMETER, ERROR_HEADER, option, arg, ERROR_FOOTER);
    }
}

/*
 * Exits if a string value is too long.
 */
void check_string_too_long(const char *param, int maxlen, const char *option)
{
    if (strlen(param) > maxlen)
    {
        error_and_exit(ERROR_PARAM_TOO_BIG, ERROR_HEADER, option, maxlen, ERROR_FOOTER);
    }
}

/*
 * Exits if a string value is too short.
 */
void check_string_too_short(const char *param, int minlen, const char *option)
{
    if (strlen(param) < minlen)
    {
        error_and_exit(ERROR_PARAM_TOO_SHORT, ERROR_HEADER, param, option, minlen, ERROR_FOOTER);
    }
}

/*
 * Checks that the num pattern lengths defined (if they are) do not exceed the maximum number of pattern lengths,
 * that the minimum pattern length is not greater than the text size.
 */
void check_num_pattern_lengths(pattern_len_info_t *pattern_info, int text_size)
{
    if (pattern_info->pattern_min_len > 0) // if set to 0, no pattern lengths are defined.
    {
        int num_pattern_lengths = get_num_pattern_lengths(pattern_info, text_size);
        if (num_pattern_lengths > NUM_PATTERNS_MAX)
        {
            error_and_exit(ERROR_TOO_MANY_PATTERN_LENGTHS, ERROR_HEADER, num_pattern_lengths,
                           pattern_info->pattern_min_len, pattern_info->pattern_max_len, pattern_info->increment_operator,
                           pattern_info->increment_by, NUM_PATTERNS_MAX, ERROR_FOOTER);
        }

        if (pattern_info->pattern_min_len > text_size)
        {
            error_and_exit(ERROR_MIN_PATTERN_BIGGER_THAN_TEXT, ERROR_HEADER, pattern_info->pattern_min_len,
                           text_size, ERROR_FOOTER);
        }
    }
}

int matches_option(const char *param, const char* short_option, const char *long_option)
{
    return !strcmpany(param, 2, short_option, long_option);
}

/*
 * Parses the next argument to the current one as an int parameter for a command option and stores it in "value".
 * Exits if there are no more parameters or if it is not an int.
 * Returns the number of parameters parsed.
 */
int parse_next_int_parameter(const char *option, int *value, int curr_arg, int argc, const char **argv)
{
    check_end_of_params(curr_arg + 1, argc, option);
    check_is_int(argv[curr_arg + 1], option);

    *value = atoi(argv[curr_arg + 1]);

    return 1;
}

/*
 * Parses the next argument to the current one as a long int parameter for a command option and stores it in "value".
 * Exits if there are no more parameters or if it is not a long value.
 * Returns the number of parameters parsed.
 */
int parse_next_long_parameter(const char *option, long *value, int curr_arg, int argc, const char **argv)
{
    check_end_of_params(curr_arg + 1, argc, option);
    check_is_int(argv[curr_arg + 1], option);

    *value = atol(argv[curr_arg + 1]);

    return 1;
}

/*
 * Parses the number of runs.
 * Returns the number of parameters parsed.
 */
int parse_num_runs(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    parse_next_int_parameter(OPTION_LONG_NUM_RUNS, &(opts->num_runs), curr_arg, argc, argv);

    return 1;
}

/*
 * Parses a text size integer parameter and converts it to mega-bytes.
 * Returns the number of parameters parsed.
 */
int parse_text_size(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    parse_next_int_parameter(OPTION_LONG_TEXT_SIZE, &(opts->text_size), curr_arg, argc, argv);
    opts->text_size *= MEGA_BYTE;

    return 1;
}

/*
 * Parses the time limit integer parameter.
 * Returns the number of parameters parsed.
 */
int parse_time_limit(run_command_opts_t *line, int curr_arg, int argc, const char **argv)
{
    parse_next_int_parameter(OPTION_LONG_MAX_TIME, &(line->time_limit_millis), curr_arg, argc, argv);

    return 1;
}

/*
 * Parses the file data source option - a list of files or folders to load into the text buffer for benchmarking.
 * Terminates at the end of parameters, or the next flag parameter.
 * Returns the number of parameters parsed.
 */
int parse_text(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    check_end_of_params(curr_arg + 1, argc, OPTION_LONG_TEXT_SOURCE);

    if (opts->data_source == DATA_SOURCE_RANDOM)
        error_and_exit(ERROR_MUTUALLY_EXCLUSIVE,
                       ERROR_HEADER, OPTION_LONG_TEXT_SOURCE, OPTION_LONG_RANDOM_TEXT, ERROR_FOOTER);

    int name_arg = 0;
    while (name_arg < MAX_DATA_SOURCES && curr_arg + name_arg + 1 < argc && argv[curr_arg + name_arg + 1][0] != '-')
    {
        opts->data_sources[name_arg] = argv[curr_arg + name_arg + 1];
        name_arg++;
    }

    if (curr_arg + name_arg + 1 < argc && !is_command_option(argv[curr_arg + name_arg + 1]))
    {
        error_and_exit(ERROR_TOO_MANY_DATA_SOURCES_DEFINED, ERROR_HEADER, OPTION_LONG_TEXT_SOURCE, MAX_DATA_SOURCES, ERROR_FOOTER);
    }

    if (name_arg > 0)
    {
        opts->data_source = DATA_SOURCE_FILES;
    } else {
        error_and_exit(ERROR_PARAMS_NOT_PROVIDED, ERROR_HEADER, OPTION_LONG_TEXT_SOURCE, ERROR_FOOTER);
    }

    return name_arg;
}

/*
 * Parses the random text data source parameter, taking an alphabet size between 1 and 256 to generate with.
 * Returns the number of parameters parsed.
 */
int parse_random_text(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    if (opts->data_source == DATA_SOURCE_FILES)
    {
        error_and_exit(ERROR_MUTUALLY_EXCLUSIVE, ERROR_HEADER, OPTION_LONG_RANDOM_TEXT, OPTION_LONG_TEXT_SOURCE, ERROR_FOOTER);
    }

    parse_next_int_parameter(OPTION_LONG_RANDOM_TEXT, &(opts->alphabet_size), curr_arg, argc, argv);
    if (opts->alphabet_size < 1 || opts->alphabet_size > 256)
    {
        error_and_exit(ERROR_INTEGER_NOT_IN_RANGE, ERROR_HEADER, OPTION_LONG_RANDOM_TEXT, 1, 256, ERROR_FOOTER);
    }
    opts->data_source = DATA_SOURCE_RANDOM;

    return 1;
}

/*
 * Parses the precision of the output - the number of decimal points to round to in the output.
 * Returns the number of parameters parsed.
 */
int parse_precision(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    parse_next_int_parameter(OPTION_LONG_PRECISION, &(opts->precision), curr_arg, argc, argv);

    return 1;
}

/*
 * Parses the pattern length parameters, setting the minimum and maxiumum bounds to search with.
 * Returns the number of parameters parsed.
 */
int parse_pattern_len(pattern_len_info_t *plen_info, int curr_arg, int argc, const char **argv)
{
    int num_params;

    parse_next_int_parameter(OPTION_LONG_PATTERN_LEN, &(plen_info->pattern_min_len), curr_arg, argc, argv);

    // If we have a second parameter at curr_arg + 2, this is the maximum
    if (curr_arg + 2 < argc && !is_command_option(argv[curr_arg + 2]))
    {
        parse_next_int_parameter(OPTION_LONG_PATTERN_LEN, &(plen_info->pattern_max_len), curr_arg + 1, argc, argv);
        if (plen_info->pattern_max_len < plen_info->pattern_min_len)
        {
            error_and_exit(ERROR_MAX_LESS_THAN_MIN, ERROR_HEADER, plen_info->pattern_max_len,
                           OPTION_LONG_PATTERN_LEN, plen_info->pattern_min_len, ERROR_FOOTER);
        }
        num_params = 2;
    }
    else // only one parameter provided - set the max to the min - just a single pattern length.
    {
        plen_info->pattern_max_len = plen_info->pattern_min_len;
        num_params = 1;
    }

    // Check we have a viable min pattern length.
    if (plen_info->pattern_min_len < 1)
        error_and_exit(ERROR_INTEGER_TOO_SMALL, ERROR_HEADER, argv[curr_arg + 1], OPTION_LONG_PATTERN_LEN, 1, ERROR_FOOTER);

    return num_params;
}

/*
 * Parses a pattern increment command, to control how pattern lengths are increased during benchmarking.
 * Syntax is + or * followed by a number, which must be at least one when adding, and 2 if multiplying.
 * The user can put a space between the operator and the digits, or not.  For example:
 *
 * -inc +1   // increment pattern length by one.
 * -inc + 2  // increment pattern length by two.
 * -inc *4   // increment pattern length by four.
 *
 * A pattern length is guaranteed to increase by at least one over the current value, no matter what the increment settings.
 * Returns the number of parameters parsed.
 */
int parse_increment(pattern_len_info_t *opts, int curr_arg, int argc, const char **argv)
{
    check_end_of_params(curr_arg, argc, OPTION_LONG_INCREMENT);
    const char *param = argv[curr_arg + 1];
    check_string_too_short(param, 1, OPTION_LONG_INCREMENT);

    int num_params_parsed = 1;
    char itype = param[0];
    if (itype == INCREMENT_ADD_OPERATOR || itype == INCREMENT_MULTIPLY_OPERATOR)
    {
        opts->increment_operator = itype;
        size_t length = strlen(param);
        if (length > 1) // the parameter supplies the digits immediately after the operator.
        {
            check_is_int(param + 1, OPTION_LONG_INCREMENT);
            opts->increment_by = atoi(param + 1);
        }
        else // the next parameter holds the digits, because the user put a space between them.
        {
            parse_next_int_parameter(OPTION_LONG_INCREMENT, &(opts->increment_by), curr_arg + 1, argc, argv);
            num_params_parsed = 2;
        }
    }
    else
    {
        error_and_exit(ERROR_INCREMENT_PLUS_OR_TIMES, ERROR_HEADER, OPTION_LONG_INCREMENT, param, ERROR_FOOTER);
    }

    int min_len = opts->increment_operator == INCREMENT_ADD_OPERATOR ? 1 : 2;
    if (opts->increment_by < min_len)
        error_and_exit(ERROR_INTEGER_TOO_SMALL, ERROR_HEADER, param, OPTION_LONG_INCREMENT, min_len, ERROR_FOOTER);

    return num_params_parsed;
}

/*
 * Parses a random seed parameter - an integer digit.
 * Returns the number of parameters parsed.
 */
int parse_seed(long *seed_value, int curr_arg, int argc, const char **argv)
{
    parse_next_long_parameter(OPTION_LONG_SEED, seed_value, curr_arg, argc, argv);

    return 1;
}

/*
 * Parses the cpu pinning options.
 * Returns the number of parameters parsed.
 */
int parse_cpu_pinning(run_command_opts_t *opts, int curr_arg, int argc, const char **argv) {
    check_end_of_params(curr_arg + 1, argc, OPTION_LONG_CPU_PIN);

    // Get a lower case version of the arg - but take a copy - don't modify arguments or environment variables...
    char param[STR_BUF];
    strcpy(param, argv[curr_arg + 1]);
    str2lower(param);
    
    if (!strcmp(param, PARAM_CPU_PINNING_OFF))
    {
        opts->cpu_pinning = PINNING_OFF;
    }
    else if (!strcmp(param, PARAM_CPU_PIN_LAST))
    {
        opts->cpu_pinning = PIN_LAST_CPU;
    }
    else if (is_int(param))
    {
        opts->cpu_pinning = PIN_SPECIFIED_CPU;
        opts->cpu_to_pin = atoi(param);
    }
    else {
        error_and_exit(ERROR_CPU_PINNING_PARAMETER,
                       ERROR_HEADER, argv[curr_arg + 1], OPTION_LONG_CPU_PIN, ERROR_FOOTER);
    }

    return 1;
}

/*
 * Parses the cpu stats.  If no additional parameters are provided, it defaults to L1 cache and branching.
 */
int parse_cpu_stats(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    int num_params_parsed = 0;

    while (next_is_param(curr_arg, argc, argv))
    {
        num_params_parsed += 1;
        const char *param = argv[++curr_arg];
        if (!strcmp(param, PARAM_CPU_STATS_FIRST_LEVEL_CACHE))
            opts->cpu_stats |= CPU_STAT_L1_CACHE;
        else if (!strcmp(param, PARAM_CPU_STATS_LAST_LEVEL_CACHE))
            opts->cpu_stats |= CPU_STAT_LL_CACHE;
        else if (!strcmp(param, PARAM_CPU_STATS_BRANCHING))
            opts->cpu_stats |= CPU_STAT_BRANCHES;
        else
            error_and_exit("Unknown parameter for cpu stats provided: %s", param);
    }

    // If no parameters set, default to l1 cache and branches.
    if (opts->cpu_stats == 0)
        opts->cpu_stats = CPU_STAT_L1_CACHE | CPU_STAT_BRANCHES;

    return num_params_parsed;
}

/*
 * Parses a run option that is not associated with any other parameter as an algo name regex.
 */
void parse_run_algo_name(run_command_opts_t *opts, int curr_arg, const char **argv)
{
    if (opts->num_algo_names < MAX_SELECT_ALGOS)
    {
        opts->algo_names[opts->num_algo_names++] = argv[curr_arg];
        opts->algo_source = ALGO_REGEXES;
    }
    else
    {
        error_and_exit("Too many algorithms specified: maxiumum number is %d", MAX_SELECT_ALGOS);
    }
}

/*
 * Parses a user supplied pattern to benchmark with.
 * Returns the number of parameters parsed.
 */
int parse_pattern(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    check_end_of_params(curr_arg + 1, argc, OPTION_LONG_PATTERN);

    opts->pattern = argv[curr_arg + 1];
    opts->pattern_info.pattern_min_len = strlen(opts->pattern);
    opts->pattern_info.pattern_max_len = opts->pattern_info.pattern_min_len;

    return 1;
}

/*
 * Parses a user supplied search data parameter
 * Returns the number of parameters parsed.
 */
int parse_search_data(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    check_end_of_params(curr_arg + 1, argc, OPTION_LONG_PATTERN);

    opts->data_source = DATA_SOURCE_USER;
    opts->data_to_search = argv[curr_arg + 1];
    opts->text_size = strlen(opts->data_to_search);

    return 1;
}

/*
 * Parses the flag switches for the run command.
 * Returns true if a flag argument was parsed.
 */
int parse_flag(run_command_opts_t *opts, int curr_arg, int argc, const char **argv)
{
    if (matches_option(argv[curr_arg], FLAG_SHORT_OCCURRENCE, FLAG_LONG_OCCURRENCE))
    {
        opts->occ = 1;
    }
    else if (matches_option(argv[curr_arg], FLAG_SHORT_PREPROCESSING_TIME, FLAG_LONG_PREPROCESSING_TIME))
    {
        opts->pre = 1;
    }
    else if (matches_option(argv[curr_arg], FLAG_SHORT_PATTERN_LENGTHS_SHORT, FLAG_LONG_PATTERN_LENGTHS_SHORT))
    {
        opts->pattern_info.pattern_min_len = 2;
        opts->pattern_info.pattern_max_len = 32;
        opts->pattern_info.increment_operator = '+';
        opts->pattern_info.increment_by = 2;
    }
    else if (matches_option(argv[curr_arg], FLAG_SHORT_PATTERN_LENGTHS_VERY_SHORT, FLAG_LONG_PATTERN_LENGTHS_VERY_SHORT))
    {
        opts->pattern_info.pattern_min_len = 1;
        opts->pattern_info.pattern_max_len = 16;
        opts->pattern_info.increment_operator = '+';
        opts->pattern_info.increment_by = 1;
    }
    else if (matches_option(argv[curr_arg], FLAG_SHORT_FILL_BUFFER, FLAG_LONG_FILL_BUFFER))
    {
        opts->fill_buffer = 1;
    }
    else if (matches_option(argv[curr_arg], FLAG_SHORT_ALL_ALGOS, FLAG_LONG_ALL_ALGOS))
    {
        opts->algo_source = ALL_ALGOS;
    }
    else
    {
        return 0; // did not match.
    }

    return 1; // matched a flag argument.
}

/*
 * Parses the named set option for the test command.
 * Returns the number of parameters parsed.
 */
int parse_test_use_named_set(test_command_opts_t *opts, const int curr_arg, const int argc, const char **argv)
{
    check_end_of_params(curr_arg + 1, argc, OPTION_LONG_USE_NAMED);
    check_is_not_a_command_option(argv[curr_arg + 1], OPTION_LONG_USE_NAMED);

    opts->named_set = argv[curr_arg + 1];
    opts->algo_source = NAMED_SET_ALGOS;

    return 1;
}

/*
 * Parses increment parameters for the test command, and sets the length defaults if not already set.
 * Returns the number of parameters parsed.
 */
int parse_test_increment(pattern_len_info_t *pattern_info, int curr_arg, int argc, const char **argv)
{
    int params_parsed = parse_increment(pattern_info, curr_arg, argc, argv);

    // If we specified an increment but haven't yet specified any pattern lengths, set the default min max values.
    if (pattern_info->pattern_min_len == 0)
    {
        pattern_info->pattern_min_len = TEST_PATTERN_MIN_LEN;
        pattern_info->pattern_max_len = TEST_PATTERN_MAX_LEN;
    }

    return params_parsed;
}

/*
 * Parses the named set test command.
 * Returns the number of parameters parsed.
 */
int parse_run_use_named_set(run_command_opts_t *opts, const int curr_arg, const int argc, const char **argv)
{
    check_end_of_params(curr_arg + 1, argc, OPTION_LONG_USE_NAMED);
    check_is_not_a_command_option(argv[curr_arg + 1], OPTION_LONG_USE_NAMED);
    check_string_too_long(argv[curr_arg + 1], STR_BUF - 8, OPTION_LONG_USE_NAMED);

    snprintf(opts->algo_filename, STR_BUF, "%.*s.algos", STR_BUF - 8, argv[curr_arg + 1]);
    opts->algo_source = NAMED_SET_ALGOS;

    return 1;
}

/*
 * Parses the main run arguments.
 */
void parse_run_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    run_command_opts_t *opts = malloc(sizeof(run_command_opts_t));
    subcommand->opts = opts;

    if (argc <= 3)
        print_run_usage_and_exit(argv[1]);

    init_run_command_opts(opts);

    for (int curr_arg = 2; curr_arg < argc; curr_arg++)
    {
        const char *param = argv[curr_arg];
        if (matches_option(param, OPTION_SHORT_NUM_RUNS, OPTION_LONG_NUM_RUNS))
        {
            curr_arg += parse_num_runs(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_TEXT_SIZE, OPTION_LONG_TEXT_SIZE))
        {
            curr_arg += parse_text_size(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_MAX_TIME, OPTION_LONG_MAX_TIME))
        {
            curr_arg += parse_time_limit(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_TEXT_SOURCE, OPTION_LONG_TEXT_SOURCE))
        {
            curr_arg += parse_text(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_RANDOM_TEXT, OPTION_LONG_RANDOM_TEXT))
        {
            curr_arg += parse_random_text(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_PATTERN_LEN, OPTION_LONG_PATTERN_LEN))
        {
            curr_arg += parse_pattern_len(&(opts->pattern_info), curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_INCREMENT, OPTION_LONG_INCREMENT))
        {
            curr_arg += parse_increment(&(opts->pattern_info), curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_SEED, OPTION_LONG_SEED))
        {
            curr_arg += parse_seed(&(opts->random_seed), curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_USE_NAMED, OPTION_LONG_USE_NAMED))
        {
            curr_arg += parse_run_use_named_set(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_PATTERN, OPTION_LONG_PATTERN))
        {
            curr_arg += parse_pattern(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_SEARCH_DATA, OPTION_LONG_SEARCH_DATA))
        {
            curr_arg += parse_search_data(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_CPU_PIN, OPTION_LONG_CPU_PIN))
        {
            curr_arg += parse_cpu_pinning(opts, curr_arg, argc, argv);
        }
        else if (matches_option(argv[curr_arg], OPTION_SHORT_GET_CPU_STATS, OPTION_LONG_GET_CPU_STATS))
        {
            curr_arg += parse_cpu_stats(opts, curr_arg, argc, argv);
        }
        else if (matches_option(argv[curr_arg], OPTION_SHORT_PRECISION, OPTION_LONG_PRECISION))
        {
            curr_arg += parse_precision(opts, curr_arg, argc, argv);
        }
        else if (!parse_flag(opts, curr_arg, argc, argv))
        {
            parse_run_algo_name(opts, curr_arg, argv);
        }
    }

    check_num_pattern_lengths(&(opts->pattern_info), opts->text_size);

    if (opts->data_source == DATA_SOURCE_NOT_DEFINED)
        error_and_exit(ERROR_NO_DATA_SOURCE_DEFINED,
                       ERROR_HEADER, OPTION_LONG_TEXT_SOURCE, OPTION_LONG_RANDOM_TEXT, ERROR_FOOTER);
}

/*
 * Parses the main test parameters.
 */
void parse_test_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    if (argc <= 2)
        print_test_usage_and_exit(argv[0]);

    test_command_opts_t *opts = malloc(sizeof(test_command_opts_t));
    init_test_command_opts(opts);

    int num_algo_names = 0;
    for (int curr_arg = 2; curr_arg < argc; curr_arg++)
    {
        const char *param = argv[curr_arg];
        if (matches_option(param, FLAG_SHORT_ALL_ALGOS, FLAG_LONG_ALL_ALGOS))
        {
            opts->algo_source = ALL_ALGOS;
        }
        else if (matches_option(param, OPTION_SHORT_TEST_SELECTED, OPTION_LONG_TEST_SELECTED))
        {
            opts->algo_source = SELECTED_ALGOS;
        }
        else if (matches_option(param, OPTION_SHORT_USE_NAMED, OPTION_LONG_USE_NAMED))
        {
            curr_arg += parse_test_use_named_set(opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_PATTERN_LEN, OPTION_LONG_PATTERN_LEN))
        {
            curr_arg += parse_pattern_len(&(opts->pattern_info), curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_INCREMENT, OPTION_LONG_INCREMENT))
        {
            curr_arg += parse_test_increment(&(opts->pattern_info), curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_SEED, OPTION_LONG_SEED))
        {
            curr_arg += parse_seed(&(opts->random_seed), curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_DEBUG, OPTION_LONG_DEBUG))
        {
            opts->debug = 1;
        }
        else if (matches_option(param, OPTION_SHORT_QUICK_TESTS, OPTION_LONG_QUICK_TESTS))
        {
            opts->test_type = QUICK_TESTS;
        }
        else if (matches_option(param, OPTION_SHORT_UPDATE, OPTION_LONG_UPDATE))
        {
            opts->test_type = FULL_TEST_AND_UPDATE;
        }
        else if (matches_option(param, OPTION_SHORT_FAIL_ONLY, OPTION_LONG_FAIL_ONLY))
        {
            opts->fail_only = 1;
        }
        else if (matches_option(param, OPTION_SHORT_HELP, OPTION_LONG_HELP))
        {
            print_test_usage_and_exit(argv[0]);
        }
        else if (is_command_option(param))
        {
            error_and_exit(ERROR_UNRECOGNISED_OPTION,
                           ERROR_HEADER, argv[1], argv[curr_arg], ERROR_FOOTER);
        }
        else
        {
            opts->algo_names[num_algo_names++] = argv[curr_arg];
        }
    }

    if (opts->algo_source == ALGO_REGEXES && num_algo_names == 0)
    {
        error_and_exit(ERROR_NO_ALGORITHMS_DEFINED, ERROR_HEADER, ERROR_FOOTER);
    }

    if (opts->pattern_info.pattern_min_len > TEST_TEXT_SIZE)
    {
        error_and_exit(ERROR_MIN_PATTERN_BIGGER_THAN_TEXT, ERROR_HEADER, opts->pattern_info.pattern_min_len,
                       TEST_TEXT_SIZE, ERROR_FOOTER);
    }

    check_num_pattern_lengths(&(opts->pattern_info), TEST_TEXT_SIZE);

    opts->num_algo_names = num_algo_names;

    subcommand->opts = (void *)opts;
}

/*
 * Parses a parameter that takes the name of a set of algorithms as the next parameter.
 */
int parse_select_named_set_parameter(const char *option, select_command_opts_t *opts , int curr_arg, int argc, const char **argv)
{
    check_end_of_params(curr_arg + 1, argc, option);
    opts->named_set = argv[curr_arg + 1];

    return 1;
}

/*
 * Parses the main select commands.
 */
void parse_select_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    select_command_opts_t *opts = malloc(sizeof(select_command_opts_t));
    init_select_command_opts(opts);

    if (argc <= 2)
        print_select_usage_and_exit(argv[0]);

    int k = 0;
    for (int curr_arg = 2; curr_arg < argc; curr_arg++)
    {
        const char *param = argv[curr_arg];
        if (matches_option(param, OPTION_SHORT_ADD, OPTION_LONG_ADD))
        {
            opts->select_command = ADD;
        }
        else if (matches_option(param, OPTION_SHORT_REMOVE, OPTION_LONG_REMOVE))
        {
            opts->select_command = REMOVE;
        }
        else if (matches_option(param, OPTION_SHORT_NO_ALGOS, OPTION_LONG_NO_ALGOS))
        {
            opts->select_command = DESELECT_ALL;
        }
        else if (matches_option(param, OPTION_SHORT_SHOW_ALL, OPTION_LONG_SHOW_ALL))
        {
            opts->select_command = SHOW_ALL;
        }
        else if (matches_option(param, OPTION_SHORT_SHOW_SELECTED, OPTION_LONG_SHOW_SELECTED))
        {
            opts->select_command = SHOW_SELECTED;
        }
        else if (matches_option(param, OPTION_SHORT_SHOW_NAMED, OPTION_LONG_SHOW_NAMED))
        {
            opts->select_command = SHOW_NAMED;
            curr_arg += parse_select_named_set_parameter(OPTION_LONG_SHOW_NAMED, opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_SAVE_AS, OPTION_LONG_SAVE_AS))
        {
            opts->select_command = SAVE_AS;
            curr_arg += parse_select_named_set_parameter(OPTION_LONG_SAVE_AS, opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_SET_DEFAULT, OPTION_LONG_SET_DEFAULT))
        {
            opts->select_command = SET_AS_DEFAULT;
            curr_arg += parse_select_named_set_parameter(OPTION_LONG_SET_DEFAULT, opts, curr_arg, argc, argv);
        }
        else if (matches_option(param, OPTION_SHORT_LIST_NAMED, OPTION_LONG_LIST_NAMED))
        {
            opts->select_command = LIST_NAMED;
        }
        else if (matches_option(param, OPTION_SHORT_HELP, OPTION_LONG_HELP))
        {
            print_select_usage_and_exit(argv[0]);
        }
        else if (is_command_option(param))
        {
            error_and_exit(ERROR_UNRECOGNISED_OPTION, ERROR_HEADER, argv[1], argv[curr_arg], ERROR_FOOTER);
        }
        else
        {
            opts->algos[k++] = argv[curr_arg];
        }
    }

    if (k == 0 && (opts->select_command == ADD || opts->select_command == REMOVE))
        print_select_usage_and_exit(argv[0]);

    if (k > 0 && !(opts->select_command == ADD || opts->select_command == REMOVE))
        print_select_usage_and_exit(argv[0]);

    opts->n_algos = k;

    subcommand->opts = (void *)opts;
}

/*
 * Parses the arguments into a valid subcommand and associated parameters.
 * It will exit with an error message if it cannot parse the arguments correctly.
 */
void parse_args(int argc, const char **argv, smart_subcommand_t *subcommand)
{
    if (argc <= 1 || !strcmpany(argv[1], 2, OPTION_SHORT_HELP, OPTION_LONG_HELP))
        print_subcommand_usage_and_exit(argv[0]);

    subcommand->subcommand = argv[1];

    if (!strcmp(argv[1], RUN_COMMAND))
    {
        parse_run_args(argc, argv, subcommand);
    }
    else if (!strcmp(argv[1], TEST_COMMAND))
    {
        parse_test_args(argc, argv, subcommand);
    }
    else if (!strcmp(argv[1], SELECT_COMMAND))
    {
        parse_select_args(argc, argv, subcommand);
    }
    else if (strcmp(argv[1], CONFIG_COMMAND))
    {
        print_subcommand_usage_and_exit(argv[0]);
    }
}

#endif