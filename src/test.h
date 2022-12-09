#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "config.h"
#include "commands.h"
#include "algorithms.h"

/*
 * Struct to hold results of testing one algorithm, and other metadata useful when testing.
 */
typedef struct test_results_info
{
    const test_command_opts_t *opts;
    char algo_name[ALGO_NAME_LEN];
    char test_message[STR_BUF];
    search_function *search_func;
    int num_tests;
    int num_passed;
    int last_expected_count;
    int last_actual_count;
    int buffer_overflow;
    int overwrites_text;
    int num_failure_messages;
    char failure_messages[MAX_FAILURE_MESSAGES][STR_BUF];
} test_results_info_t;

/*
 * Inits all fields in the test_results struct.
 */
void init_test_results(test_results_info_t *results, const test_command_opts_t *opts,
                       const char *algo_name, search_function *search_func)
{
    set_upper_case_algo_name(results->algo_name, algo_name);
    results->search_func = search_func;
    results->num_tests = 0;
    results->num_passed = 0;
    results->opts = opts;
    results->last_expected_count = -1; // initialise to an invalid value.
    results->last_actual_count = -2;   // initialise to a *different* invalid value.
    results->buffer_overflow = 0;
    results->overwrites_text = 0;
    results->num_failure_messages = 0;
    memset(results->failure_messages, 0, MAX_FAILURE_MESSAGES * STR_BUF);
    memset(results->test_message, 0, STR_BUF);
}

/*
 * Allows debugging a failing search function from a test run with the --debug flag set.
 * It will re-run any test that fails testing. Put a breakpoint in this function in your debugger,
 * and ensure you have built both smart and all algorithms with the debug CFLAGS enabled in Makefile.
 */
void debug_search(test_results_info_t *test_results, unsigned char *pattern, int m, unsigned char *T, int n)
{
    if (test_results->opts->debug)
    {
        double pre_time = 0.0;
        double search_time = 0.0;

        // Put a breakpoint here and enable the --debug flag to re-run a failing search algorithm.
        // The number of expected occurrences is found in: test_results->last_expected_count
        // The number of actual occurrences found is in:   test_results->last_actual_count
        test_results->search_func(pattern, m, T, n, &pre_time, &search_time);
    }
}

/*
 * The brute force algorithm used for comparing occurrences with algorithms being tested.
 * Returns the number of occurrences of the pattern x of length m in text y of length n.
 */
int reference_search(const unsigned char *x, int m, const unsigned char *y, int n)
{
    /* Searching */
    int count = 0;
    for (int j = 0; j <= n - m; ++j)
    {
        int i;
        for (i = 0; i < m && x[i] == y[i + j]; ++i)
            ;
        if (i >= m)
            count++;
    }

    return count;
}

/*
 * Prints the current status of testing.
 * Overwrites the current line by using a \r carriage return at the start of the line.
 */
void print_test_status(test_results_info_t *test_results, int percent_done, const char *message)
{
    snprintf(test_results->test_message, STR_BUF, "\r\tTesting %-*s [%.2d%%]     %-24s  (%d/%d)      ",
             ALGO_NAME_LEN, test_results->algo_name, percent_done, message, test_results->num_passed, test_results->num_tests);
    printf("%s", test_results->test_message);
    fflush(stdout);
}

/*
 * Runs a single test on a single algorithm and places the results in test_results.
 * Returns true (1) if the test passed.
 */
int test_algo(unsigned char *pattern, const int m, unsigned char *data, const int n, test_results_info_t *test_results)
{
    // We assume a test passes and fail it explicitly.
    // If an algorithm cannot search a pattern, it is also a "pass", in that no failure has been detected.
    // The actual number of tests that run and how many passed is captured in the test_results struct.
    int test_passed = 1;

    // We don't need these in testing, but we still initialise them to avoid valgrind warnings or subtle bugs.
    double search_time = 0;
    double pre_time = 0;

    // Get the result for the algorithm being tested:
    test_results->last_actual_count = test_results->search_func(pattern, m, data, n, &search_time, &pre_time);

    // Get the reference expected count using a brute force search:
    test_results->last_expected_count = reference_search(pattern, m, data, n);

    // if we have a count, add to test results. Zero can be a valid count in test.
    // -2 means the search encountered an error while running, so we flag that as a failed test.
    // -1 means algo can't search that pattern, so we ignore the test.7
    if (test_results->last_actual_count >= 0 || test_results->last_actual_count == ERROR_SEARCHING)
    {
        test_results->num_tests += 1;
        if (test_results->last_actual_count == test_results->last_expected_count)
        {
            test_results->num_passed += 1;
        }
        else
        {
            test_passed = 0; // test failed.
        }
    }

    return test_passed;
}

/*
 * Adds a formatted failure message to the test results for an algorithm, up to the maximum number of failure messages.
 */
void add_failure_message(test_results_info_t *test_results, const char *format, ...)
{
    if (test_results->num_failure_messages < MAX_FAILURE_MESSAGES)
    {
        va_list args;
        va_start(args, format);
        vsnprintf(test_results->failure_messages[test_results->num_failure_messages++], STR_BUF, format, args);
        va_end(args);
    }
}

/*
 * Runs a test on a fixed string and text and outputs information about any failure.
 */
void test_fixed_string(char *pattern, char *text, test_results_info_t *test_results)
{
    int m = strlen(pattern);
    int n = strlen(text);

    unsigned char search_data[get_text_buffer_size(n, m)];
    memcpy(search_data, text, n);
    memset(search_data + n, 0, m * 2);

    // Cast because C strings are char but search function is defined with unsigned char.
    if (!test_algo((unsigned char *)pattern, m, search_data, n, test_results))
    {
        if (test_results->last_actual_count == ERROR_SEARCHING)
        {
            add_failure_message(test_results, "Algorithm reported an error while processing. Fixed pattern tests searching '%s' in '%s'",
                                pattern, text);
        }
        else
        {
            add_failure_message(test_results, "Found %d of %d occurrences. Fixed pattern tests searching '%s' in '%s'",
                                test_results->last_actual_count, test_results->last_expected_count, pattern, text);
        }

        debug_search(test_results, (unsigned char *)pattern, m, search_data, n);
    }
}

/*
 * Short fixed length string tests.
 *
 */
void run_fixed_tests(test_results_info_t *test_results)
{
    print_test_status(test_results, 2, "Fixed patterns");
    test_fixed_string("aa", "aaaaaaaaaa", test_results);
    test_fixed_string("a", "aaaaaaaaaa", test_results);
    test_fixed_string("aaaaaaaaaa", "aaaaaaaaaa", test_results);
    test_fixed_string("b", "aaaaaaaaaa", test_results);
    test_fixed_string("abab", "ababababab", test_results);
    test_fixed_string("aba", "ababababab", test_results);
    test_fixed_string("abc", "ababababab", test_results);
    test_fixed_string("ba", "ababababab", test_results);
    test_fixed_string("babbbbb", "ababababab", test_results);
    test_fixed_string("bcdefg", "bcdefghilm", test_results);
    test_fixed_string("babbbbb", "abababbbbb", test_results);
    test_fixed_string("bababb", "abababbbbb", test_results);
    test_fixed_string("xyz123", "abcxy123yz123wxyz123", test_results);
}

/*
 * Gets a random pattern from the text T.
 * Returns the length of the pattern.
 */
int get_random_pattern_from_text(unsigned char pattern[TEST_PATTERN_MAX_LEN], unsigned char *T)
{
    int pat_len = (rand() % (TEST_PATTERN_MAX_LEN - 1)) + 1;
    pat_len = pat_len < 1 ? 1 : pat_len;
    int start = rand() % (TEST_PATTERN_MAX_LEN - pat_len);
    memcpy(pattern + start, T, pat_len);
    return pat_len;
}

/*
 * Produces random consecutive patterns in the text by finding a random pattern and then
 * duplicating it immediately after the original pattern.
 * Returns the length of the pattern.
 */
int gen_consecutive_pattern_in_text(unsigned char pattern[TEST_PATTERN_MAX_LEN], unsigned char *T)
{
    int pat_len = rand() % (TEST_PATTERN_MAX_LEN - 1);
    pat_len = pat_len < 1 ? 1 : pat_len;
    int start = rand() % (TEST_TEXT_SIZE - (pat_len * 2));
    memcpy(pattern, T + start, pat_len); // grab a pattern from the text.
    memcpy(T + start + pat_len, pattern, pat_len); // put a copy of the pattern after the one we just found.
    return pat_len;
}

/*
 * Produces random nearly consecutive patterns in the text by finding a random pattern and then
 * duplicating it immediately after the original pattern, but with the first char missing of the second pattern.
 * Returns the length of the pattern.
 */
int gen_partial_overlapping_pattern_in_text(unsigned char pattern[TEST_PATTERN_MAX_LEN], unsigned char *T)
{
    int pat_len = rand() % (TEST_PATTERN_MAX_LEN - 1);
    pat_len = pat_len < 1 ? 1 : pat_len;
    int start = rand() % (TEST_TEXT_SIZE - (pat_len * 2));
    memcpy(pattern, T + start, pat_len); // grab a pattern from the text.
    memcpy(T + start + pat_len, pattern + 1, pat_len - 1); // put a partial copy of the pattern after the one we just found.
    return pat_len;
}

/*
 * Runs one random test for an algorithm and outputs information about failures.
 * Returns true (1) if the test passed or if the test was ignored, and 0 if the test executed and failed.
 */
int run_random_test(test_results_info_t *test_results, unsigned char *pattern, int m, unsigned char *T,
                    const char *test_description, int sigma)
{
    int passed = test_algo(pattern, m, T, TEST_TEXT_SIZE, test_results);
    if (!passed)
    {
        if (test_results->last_actual_count == ERROR_SEARCHING)
        {
            add_failure_message(test_results, "Algorithm reported an error while processing. %s tests (alphabet: %d, pattern length: %d, random seed: %ld))",
                                test_description, sigma, m, test_results->opts->random_seed);
        }
        else
        {
            add_failure_message(test_results, "Found %d of %d occurrences. %s tests (alphabet: %d, pattern length: %d, random seed: %ld)",
                                test_results->last_actual_count, test_results->last_expected_count, test_description, sigma, m, test_results->opts->random_seed);
        }

        debug_search(test_results, pattern, m, T, TEST_TEXT_SIZE);
    }

    return passed;
}

/*
 * Tests several random patterns selected from the text.
 */
int test_random_patterns(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;
    unsigned char pattern[TEST_PATTERN_MAX_LEN];

    // Test random patterns extracted from the text:
    for (int test_no = 1; test_no <= 8; test_no++)
    {
        // Test pattern which exists in the text:
        int m = get_random_pattern_from_text(pattern, T);
        passed &= run_random_test(test_results, pattern, m, T, "Random position", sigma);

        if (!passed)
            break;
    }

    return passed;
}

/*
 * Tests several random patterns selected from the text, but with the first character of the pattern
 * altered with XOR so it no longer matches the original pattern.
 *
 * This test helps to find algorithms that don't handle the very end of a pattern properly when matching or searching.
 */
int test_bad_first_char_patterns(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;
    unsigned char pattern[TEST_PATTERN_MAX_LEN];

    // Test pattern which may not exist in the text that modifies the first character:
    for (int test_no = 1; test_no <= 8; test_no++)
    {
        int m = get_random_pattern_from_text(pattern, T);
        pattern[0] ^= pattern[0];
        passed &= run_random_test(test_results, pattern, m, T, "First char modified", sigma);

        if (!passed)
            break;
    }

    return passed;
}

/*
 * Tests patterns of length 1 to 12 that are found at the very start of a text.
 *
 * This helps to find algorithms that don't handle the start of text properly.
 */
int test_patterns_at_start(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;
    unsigned char pattern[TEST_PATTERN_MAX_LEN];

    // Test short patterns from start of text:
    for (int pat_len = 1; pat_len < 12; pat_len++)
    {
        memcpy(pattern, T, pat_len);
        passed &= run_random_test(test_results, pattern, pat_len, T, "Patterns at the start", sigma);
        if (!passed)
            break;
    }

    return passed;
}

/*
 * Tests patterns of length 1 to 12 that are found near the start of a text,
 * from 1 from the start up to 12 from the start.
 *
 * This helps to find algorithms that don't handle matches close to the start of a text properly.
 */
int test_patterns_near_start(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;
    unsigned char pattern[TEST_PATTERN_MAX_LEN];

    // Test short patterns near start of text:
    for (int pat_len = 1; pat_len < 12; pat_len++)
    {
        memcpy(pattern, T + pat_len, pat_len);
        passed &= run_random_test(test_results, pattern, pat_len, T, "Patterns near start", sigma);
        if (!passed)
            break;
    }

    return passed;
}

/*
 * Tests patterns of length 1 to 12 that are found at the end of a text.
 *
 * This helps to find algorithms that don't handle the end of text properly.
 */
int test_patterns_at_end(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;
    unsigned char pattern[TEST_PATTERN_MAX_LEN];

    // Test short patterns at end of text:
    for (int pat_len = 1; pat_len < 12; pat_len++)
    {
        memcpy(pattern, T + TEST_TEXT_SIZE - pat_len, pat_len);
        passed &= run_random_test(test_results, pattern, pat_len, T, "Patterns at the end", sigma);
        if (!passed)
            break;
    }

    return passed;
}

/*
 * Tests patterns of length 1 to 12 that are found near the end of a text,
 * from 1 from the end up to 12 from the end.
 *
 * This helps to find algorithms that don't handle matches close to the end of a text properly.
 */
int test_patterns_near_end(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;
    unsigned char pattern[TEST_PATTERN_MAX_LEN];

    // Test short patterns near end of text:
    for (int pat_len = 1; pat_len < 12; pat_len++)
    {
        memcpy(pattern, T + TEST_TEXT_SIZE - (pat_len * 2), pat_len);
        passed &= run_random_test(test_results, pattern, pat_len, T, "Patterns near end", sigma);
        if (!passed)
            break;
    }

    return passed;
}

/*
 * Tests two identical patterns next to one another in the text.
 * Selects a random pattern from the text, and then copies to the text immediately after it.
 *
 * This helps to find algorithms that shift forwards too much after they find a match, or are
 * otherwise confused by two matches one after another.
 */
int test_consecutive_patterns(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;
    unsigned char pattern[TEST_PATTERN_MAX_LEN];

    // Test consecutive pattern matches (modifies the text)
    for (int num_to_test = 0; num_to_test < 8; num_to_test++)
    {
        int m = gen_consecutive_pattern_in_text(pattern, T);
        passed &= run_random_test(test_results, pattern, m, T, "Consecutive pattern", sigma);
        if (!passed)
            break;
    }

    return passed;
}

/*
 * Tests two patterns next to one another.  The first pattern is a random pattern selected from the text.
 * The second pattern is a copy of the first pattern, but missing its first character.  This is then
 * copied to the text immediately after the original pattern.
 *
 * This helps to find algorithms which are confused after a match and with a very close but not correct match immediately afterwards.
 */
int test_consecutive_partial_patterns(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;
    unsigned char pattern[TEST_PATTERN_MAX_LEN];

    // Test partial consecutive pattern (only a partial pattern after the first one, missing the first char of the pattern).
    for (int num_to_test = 0; num_to_test < 8; num_to_test++) // (modifies the text)
    {
        int m = gen_partial_overlapping_pattern_in_text(pattern, T);
        passed &= run_random_test(test_results, pattern, m, T, "Partial consecutive pattern", sigma);
        if (!passed)
            break;
    }

    return passed;
}

/*
 * Updates the test percentage and prints it if it has changed, given a value done between 0 and 1.
 */
int update_random_test_percentage(test_results_info_t *test_results, double done, int start_percent, int last_percent)
{
    int percent_done = start_percent + (int) (done * (100 - start_percent));
    if (percent_done != last_percent)
    {
        print_test_status(test_results, percent_done, "Randomised tests");
    }

    return percent_done;
}

/*
 * Runs a lot of randomised tests on the algorithms.
 */
void run_random_tests(test_results_info_t *test_results, unsigned char *T)
{
    int start_percent = 3;  // buffer overflow is 1, and fixed tests is 2 as a very rough estimate.
    int percent_done = start_percent;
    print_test_status(test_results, percent_done, "Randomised tests");

    // Always start with same random seed for each algorithm.
    // Ensures that different sets of algorithms will not affect the test results for each algorithm.
    srand(test_results->opts->random_seed);

    int start_alphabet = test_results->opts->quick ? 64 : 1;
    int end_alphabet   = test_results->opts->quick ? 128 : 256;
    int range = end_alphabet - start_alphabet;
    int increment      = test_results->opts->quick ? 4 : 1;

    // Run tests with random text of all alphabets.
    for (int sigma = start_alphabet; sigma <= end_alphabet; sigma += increment)
    {
        double done = (double)(sigma - start_alphabet) / range;
        percent_done = update_random_test_percentage(test_results, done, start_percent, percent_done);

        gen_random_text(sigma, T, TEST_TEXT_SIZE);
        unsigned char pattern[TEST_PATTERN_MAX_LEN] = {0};

        int passed = 1;
        passed &= test_random_patterns(test_results, T, sigma);
        passed &= test_bad_first_char_patterns(test_results, T, sigma);
        passed &= test_patterns_at_start(test_results, T, sigma);
        passed &= test_patterns_near_start(test_results, T, sigma);
        passed &= test_patterns_at_end(test_results, T, sigma);
        passed &= test_patterns_near_end(test_results, T, sigma);
        passed &= test_consecutive_patterns(test_results, T, sigma);
        passed &= test_consecutive_partial_patterns(test_results, T, sigma);

        if (!passed)
        {
            break; // don't test more alphabets if we have failures in this one.
        }
    }
}

/*
 * Tests an algorithm to detect if it writes to the text buffer, and if so, does it exceed the normal buffer limit.
 * Returns true (1) if the test passed, and (0) if there was a buffer overflow failure.
 */
int run_buffer_overflow_tests(test_results_info_t *test_results)
{
    test_results->num_tests++;

    print_test_status(test_results, 1, "Buffer overflow tests");
    int passed = 1;

    // Allocate a larger buffer than normal, so we have some room to detect overflows without crashing ourselves...
    int buffer_size = TEST_TEXT_SIZE * 4;
    unsigned char search_data[buffer_size];
    unsigned char copy_data[buffer_size];

    // Set a pattern
    unsigned char pattern[TEST_PATTERN_MAX_LEN];
    memset(pattern, 1, TEST_PATTERN_MAX_LEN);

    // all zeros in the search data.
    memset(search_data, 0, TEST_TEXT_SIZE);

    // random data past the search data.
    gen_random_text(256, search_data + TEST_TEXT_SIZE, buffer_size - TEST_TEXT_SIZE);

    // take a copy of the search data to check later:
    memcpy(copy_data, search_data, buffer_size);

    // Test the algorithm.
    run_random_test(test_results, pattern, TEST_PATTERN_MAX_LEN, search_data, "Mismatched pattern", 256);

    // Test that the actual search text is not modified:
    for (int i = 0; i < TEST_TEXT_SIZE; i++)
    {
        if (copy_data[i] != search_data[i])
        {
            add_failure_message(test_results, "Overwrote the search text at position %s in a text of size %d",
                                i, TEST_TEXT_SIZE);
            test_results->overwrites_text = 1;
            passed = 0; // overwriting the text itself is a major failure.

            debug_search(test_results, pattern, TEST_PATTERN_MAX_LEN, search_data, TEST_TEXT_SIZE);
            break;
        }
    }

    // Test whether data after the supported buffer size has been modified.
    int supported_buffer_size = get_text_buffer_size(TEST_TEXT_SIZE, TEST_PATTERN_MAX_LEN);
    for (int i = buffer_size - 1; i >= supported_buffer_size; i--)
    {
        if (copy_data[i] != search_data[i])
        {
            add_failure_message(test_results, "Overwrote the buffer beyond the supported buffer size.");
            test_results->buffer_overflow = 1;
            passed = 0; // overwriting text beyond the max buffer size supported is a major failure.

            debug_search(test_results, pattern, TEST_PATTERN_MAX_LEN, search_data, TEST_TEXT_SIZE);
            break;
       }
    }

    if (passed)
    {
        test_results->num_passed++;
    }

    return passed;
}

/*
 * Prints the final results of testing an algo.
 */
void print_test_results(test_results_info_t *test_results)
{
    // Print the final status:
    if (test_results->overwrites_text)
    {
        printf("\r\tTested  %-*s [ERROR]   Overwrote text.          (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
               test_results->num_passed, test_results->num_tests);
    }
    else if (test_results->buffer_overflow)
    {
        printf("\r\tTested  %-*s [ERROR]   Buffer overflow.          (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
               test_results->num_passed, test_results->num_tests);
    }
    else if (test_results->num_tests == 0)
    {
        printf("\r\tTested  %-*s [--]      No tests executed.        (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
               test_results->num_passed, test_results->num_tests);
    }
    else if (test_results->num_passed == test_results->num_tests)
    {
        printf("\r\tTested  %-*s [OK]      All passed                (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
             test_results->num_passed, test_results->num_tests);
    }
    else if (test_results->num_passed == 0)
    {
        printf("\r\tTested  %-*s [FAIL]    None passed               (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
             test_results->num_passed, test_results->num_tests);
    }
    else
    {
        printf("\r\tTested  %-*s [FAIL]    Some failed               (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
             test_results->num_passed, test_results->num_tests);
    }

    // Report failure messages, if any.
    for (int i = 0; i < test_results->num_failure_messages; i++)
    {
        printf("\t        %-*s           %s\n", ALGO_NAME_LEN, test_results->algo_name, test_results->failure_messages[i]);
    }
}

/*
 * Tests all the algorithms with buffer overflow, fixed and random tests.
 *
 * Different algorithms will end up running different numbers of tests, since some algorithms cannot search for
 * some patterns, e.g. an algorithm using qgrams of length 4 cannot search for patterns less than 4 in length.
 *
 * The number of tests which actually ran, and the number of passing tests is recorded for each algorithm in the test results struct.
 */
void test_algos(const test_command_opts_t *opts, const algo_info_t *algorithms)
{
    if (algorithms->num_algos > 0)
    {
        unsigned char *T = (unsigned char *)malloc(get_text_buffer_size(TEST_TEXT_SIZE, TEST_PATTERN_MAX_LEN));

        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            test_results_info_t test_results;
            init_test_results(&test_results, opts, algorithms->algo_names[algo_no], algorithms->algo_functions[algo_no]);

            print_test_status(&test_results, 0, "");

            // Test for buffer overflows first; if we find one we don't run the other tests for the algorithm.
            if (run_buffer_overflow_tests(&test_results))
            {
                run_fixed_tests(&test_results);
                run_random_tests(&test_results, T);
            }

            print_test_results(&test_results);
        }

        free(T);
    }
    else
    {
        warn("No algorithms were provided to test.\n");
    }
}

/*
 * Merges any algorithms specified at the command line with ones loaded from selected or another named set.
 */
void merge_regex_algos(const smart_config_t *smart_config, const test_command_opts_t *opts, algo_info_t *algorithms)
{
    if (opts->num_algo_names > 0)
    {
        algo_info_t regex_algos;
        get_all_algo_names(smart_config, &regex_algos);
        filter_out_names_not_matching_regexes(&regex_algos, NULL, opts->algo_names, opts->num_algo_names);
        merge_algorithms(algorithms, &regex_algos, NULL);
    }
}

/*
 * Gets the set of algorithm names to test with.
 */
void get_algonames_to_test(algo_info_t *algorithms, const test_command_opts_t *opts, const smart_config_t *smart_config)
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
    case SELECTED_ALGOS:
    {
        read_algo_names_from_file(smart_config, algorithms, SELECTED_ALGOS_FILENAME);
        merge_regex_algos(smart_config, opts, algorithms);
        break;
    }
    case NAMED_SET_ALGOS:
    {
        char set_filename[STR_BUF];
        snprintf(set_filename, STR_BUF, "%s.algos", opts->named_set);
        read_algo_names_from_file(smart_config, algorithms, set_filename);
        merge_regex_algos(smart_config, opts, algorithms);
        break;
    }
    case ALL_ALGOS:
    {
        get_all_algo_names(smart_config, algorithms);
        break;
    }

    default:
    {
        error_and_exit("Unknown algorithm source specified: %d\n", opts->algo_source);
    }
    }

    // Sort the names before loading and testing. Don't sort names *after* loading the algo shared libraries,
    // or the names won't correspond to the function pointers or handles in the algorithm struct.
    if (algorithms->num_algos > 1)
    {
        sort_algorithm_names(algorithms);
    }
}

/*
 * Runs tests against the algorithms selected by the test command.
 */
void run_tests(const smart_config_t *smart_config, const test_command_opts_t *opts)
{
    algo_info_t algorithms;
    get_algonames_to_test(&algorithms, opts, smart_config);
    load_algo_shared_libraries(smart_config, &algorithms);
    print_algorithms_as_list("\tTesting ", &algorithms);
    printf("\n");

    if (opts->quick)
    {
        warn("Running quick tests - these results are not as reliable but give faster feedback.\n");
    }

    char time_format[26];
    set_time_string(time_format, 26, "%Y:%m:%d %H:%M:%S");
    info("Algorithm correctness tests started at %s", time_format);

    test_algos(opts, &algorithms);

    set_time_string(time_format, 26, "%Y:%m:%d %H:%M:%S");
    info("Algorithm correctness tests finished at %s", time_format);

    unload_algos(&algorithms);
}

/*
 * Main method to set things up before executing benchmarks with the benchmarking options provided.
 * Returns 0 if successful.
 */
int init_and_run_tests(const test_command_opts_t *test_opts, const smart_config_t *smart_config)
{
    print_logo();

    set_random_seed(test_opts->random_seed);

    run_tests(smart_config, test_opts);

    return 0;
}

/*
 * Runs the appropriate test command.
 */
int exec_test(const test_command_opts_t *test_opts, const smart_config_t *smart_config)
{
    init_and_run_tests(test_opts, smart_config);
}


