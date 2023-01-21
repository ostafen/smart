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
    int num_failures;
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
    results->num_failures = 0;
    memset(results->failure_messages, 0, MAX_FAILURE_MESSAGES * STR_BUF);
    memset(results->test_message, 0, STR_BUF);
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
        int expected_count = reference_search(pattern, m, T, n);
        test_results->search_func(pattern, m, T, n, &pre_time, &search_time);
    }
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
    if (test_results->num_failures < MAX_FAILURE_MESSAGES)
    {
        va_list args;
        va_start(args, format);
        vsnprintf(test_results->failure_messages[test_results->num_failures++], STR_BUF, format, args);
        va_end(args);
    }
}

/*
 * Runs a test on a fixed string and text and outputs information about any failure.
 * Returns whether the test passed.
 */
int test_fixed_string(char *pattern, char *text, test_results_info_t *test_results)
{
    int passed = 1;

    int m = strlen(pattern);
    int n = strlen(text);
    int buffer_size = TEST_TEXT_PRE_BUFFER + get_text_buffer_size(n, m);

    // Try to avoid stack corruption due to buggy algorithms - take a copy of the data to test and allocate on the heap:
    unsigned char *pattern_data = (unsigned char *)malloc(sizeof(unsigned char) * (m + 1));
    unsigned char *text_data = (unsigned char *)malloc(sizeof(unsigned char) * buffer_size);

    // Copy pattern and search text to heap memory, and zero out remaining text.
    memcpy(pattern_data, pattern, m);
    pattern_data[m] = '\0';
    memset(text_data, 0, TEST_TEXT_PRE_BUFFER);
    memcpy(text_data + TEST_TEXT_PRE_BUFFER, text, n);
    memset(text_data + TEST_TEXT_PRE_BUFFER + n, 0, buffer_size - TEST_TEXT_PRE_BUFFER - n);

    if (!test_algo(pattern_data, m, text_data + TEST_TEXT_PRE_BUFFER, n, test_results))
    {
        passed = 0;

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

        debug_search(test_results, pattern_data, m, text_data + TEST_TEXT_PRE_BUFFER, n);
    }

    free(pattern_data);
    free(text_data);

    return passed;
}

/*
 * Short fixed length string tests.
 */
int run_fixed_tests(test_results_info_t *test_results)
{
    int passed = 1;

    print_test_status(test_results, 2, "Fixed patterns");

    passed &= test_fixed_string("aa", "aaaaaaaaaa", test_results);
    passed &= test_fixed_string("a", "aaaaaaaaaa", test_results);
    passed &= test_fixed_string("aaaaaaaaaa", "aaaaaaaaaa", test_results);
    passed &= test_fixed_string("b", "aaaaaaaaaa", test_results);
    passed &= test_fixed_string("abab", "ababababab", test_results);
    passed &= test_fixed_string("aba", "ababababab", test_results);
    passed &= test_fixed_string("abc", "ababababab", test_results);
    passed &= test_fixed_string("ba", "ababababab", test_results);
    passed &= test_fixed_string("babbbbb", "ababababab", test_results);
    passed &= test_fixed_string("bcdefg", "bcdefghilm", test_results);
    passed &= test_fixed_string("babbbbb", "abababbbbb", test_results);
    passed &= test_fixed_string("bababb", "abababbbbb", test_results);
    passed &= test_fixed_string("xyz123", "abcxy123yz123wxyz123", test_results);

    return passed;
}

/*
 * Returns a random value between from and to inclusive.
 */
int get_random_value_between(int from, int to)
{
    int length = to - from;
    return length > 0 ? rand() % length + from : from;
}

/*
 * Returns a random position in the text for a pattern of length m.
 * If the pattern size is bigger than the text, it just returns a start position of zero.
*/
int get_random_position_in_text(const int m)
{
    return get_random_value_between(0, TEST_TEXT_SIZE - m);
}

/*
 * Gets a random pattern from the text T of size TEST_TEXT_SIZE,
 * with a specified length within the bounds of TEST_PATTERN_MIN_LEN to TEST_PATTERN_MAX_LEN.
 * Returns the actual length of the pattern obtained.
 */
int get_random_pattern_from_text_with_length(unsigned char *pattern, unsigned char *T, int pat_len)
{
    int max_len = MIN(TEST_PATTERN_MAX_LEN, TEST_TEXT_SIZE);
    pat_len = WITHIN(pat_len, TEST_PATTERN_MIN_LEN, max_len);
    memcpy(pattern, T + get_random_position_in_text(pat_len), pat_len);
    return pat_len;
}

/*
 * Gets a pattern of random length and random position from the text T.
 * Returns the length of the pattern obtained.
 */
int get_random_pattern_from_text(unsigned char *pattern, unsigned char *T)
{
    int max_len = MIN(TEST_PATTERN_MAX_LEN, TEST_TEXT_SIZE);
    return get_random_pattern_from_text_with_length(pattern, T, get_random_value_between(TEST_PATTERN_MIN_LEN, max_len));
}

/*
 * Produces random consecutive patterns in the text by finding a random pattern and then
 * duplicating it immediately after the original pattern.
 * Returns the length of the pattern.
 */
int gen_consecutive_pattern_in_text(unsigned char *pattern, unsigned char *T)
{
    int max_len = TEST_TEXT_SIZE - (TEST_PATTERN_MAX_LEN * 2) > 0 ? TEST_PATTERN_MAX_LEN : TEST_TEXT_SIZE / 2;
    int pat_len = get_random_value_between(TEST_PATTERN_MIN_LEN, max_len);
    int position = get_random_position_in_text(pat_len * 2);
    memcpy(pattern, T + position, pat_len); // grab a pattern from the text.
    memcpy(T + position + pat_len, pattern, pat_len); // put a copy of the pattern after the one we just found.
    return pat_len;
}

/*
 * Produces random nearly consecutive patterns in the text by finding a random pattern and then
 * duplicating it immediately after the original pattern, but with the first char missing of the second pattern.
 * Returns the length of the pattern.
 */
int gen_partial_overlapping_pattern_in_text(unsigned char *pattern, unsigned char *T) {
    int max_len = TEST_TEXT_SIZE - (TEST_PATTERN_MAX_LEN * 2) > 0 ? TEST_PATTERN_MAX_LEN : TEST_TEXT_SIZE / 2;
    int pat_len = get_random_value_between(TEST_PATTERN_MIN_LEN, max_len);
    int position = get_random_position_in_text(pat_len * 2);
    memcpy(pattern, T + position, pat_len); // grab a pattern from the text.
    memcpy(T + position + pat_len, pattern + 1,pat_len - 1); // put a partial copy of the pattern after the one we just found.
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
 * Tests random patterns with specified lengths from the text.
 * Returns True if passed.
 */
int test_random_patterns_with_specific_lengths(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    unsigned char *pattern = (unsigned char *)calloc(TEST_PATTERN_MAX_LEN + 1, sizeof(unsigned char));
    int passed = 1;

    const pattern_len_info_t *pattern_info = &(test_results->opts->pattern_info);

    int max_pattern_length = get_max_pattern_length(pattern_info, TEST_TEXT_SIZE);

    for (int pattern_length = pattern_info->pattern_min_len; pattern_length <= max_pattern_length;
         pattern_length = next_pattern_length(pattern_info, pattern_length))
    {
        // Test pattern which exists in the text with the current length:
        int m = get_random_pattern_from_text_with_length(pattern, T, pattern_length);
        passed &= run_random_test(test_results, pattern, m, T, "Specified lengths", sigma);

        if (!passed)
            break;
    }

    free(pattern);
    return passed;
}

/*
 * Tests randomly selected patterns with random lengths from the text.
 * Returns True if passed.
 */
int test_random_patterns_with_random_lengths(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    unsigned char *pattern = (unsigned char *)calloc(TEST_PATTERN_MAX_LEN + 1, sizeof(unsigned char));
    int passed = 1;

    int num_tests = test_results->opts->test_type == QUICK_TESTS ? TEST_QUICK_ITERATIONS : TEST_ITERATIONS;
    for (int test_no = 1; test_no <= num_tests; test_no++)
    {
        // Test pattern which exists in the text with a random length.
        int m = get_random_pattern_from_text(pattern, T);
        passed &= run_random_test(test_results, pattern, m, T, "Random lengths", sigma);

        if (!passed)
            break;
    }

    free(pattern);
    return passed;
}

/*
 * Tests several random patterns selected from the text.
 * If pattern length control was provided with -inc or -plen, then use those pattern lengths,
 * otherwise select the pattern lengths randomly.  The pattern_min_len will be greater than zero if there
 * are specified pattern lengths.
 *
 * Returns True if passed.
 */
int test_random_patterns(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    if (test_results->opts->pattern_info.pattern_min_len > 0)
    {
        return test_random_patterns_with_specific_lengths(test_results, T, sigma);
    }
    else {
        return test_random_patterns_with_random_lengths(test_results, T, sigma);
    }
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
    unsigned char *pattern = (unsigned char *)calloc(TEST_PATTERN_MAX_LEN + 1, sizeof(unsigned char));

    // Test pattern which may not exist in the text that modifies the first character:
    int num_tests = test_results->opts->test_type == QUICK_TESTS ? TEST_QUICK_ITERATIONS : TEST_ITERATIONS;
    for (int test_no = 1; test_no <= num_tests; test_no++)
    {
        int m = get_random_pattern_from_text(pattern, T);
        pattern[0] ^= pattern[0];
        passed &= run_random_test(test_results, pattern, m, T, "First char modified", sigma);

        if (!passed)
            break;
    }

    free(pattern);
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

    int max_pat_len = MIN(TEST_PATTERN_MAX_LEN, TEST_SHORT_PAT_LEN);
    unsigned char *pattern = (unsigned char *)calloc(max_pat_len + 1, sizeof(unsigned char));

    // Test short patterns from start of text:
    for (int pat_len = 1; pat_len < max_pat_len; pat_len++)
    {
        memcpy(pattern, T, pat_len);
        passed &= run_random_test(test_results, pattern, pat_len, T, "Patterns at the start", sigma);

        if (!passed)
            break;
    }

    free(pattern);
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

    int max_pat_len = MIN(TEST_PATTERN_MAX_LEN, TEST_SHORT_PAT_LEN);
    unsigned char *pattern = (unsigned char *)calloc(max_pat_len + 1, sizeof(unsigned char));

    // Test short patterns near start of text:
    for (int pat_len = 1; pat_len < max_pat_len; pat_len++)
    {
        memcpy(pattern, T + pat_len, pat_len);
        passed &= run_random_test(test_results, pattern, pat_len, T, "Patterns near start", sigma);

        if (!passed)
            break;
    }

    free(pattern);
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

    int max_pat_len = MIN(TEST_PATTERN_MAX_LEN, TEST_SHORT_PAT_LEN);
    unsigned char *pattern = (unsigned char *)calloc(max_pat_len + 1, sizeof(unsigned char));

    // Test short patterns at end of text:
    for (int pat_len = 1; pat_len < max_pat_len; pat_len++)
    {
        memcpy(pattern, T + TEST_TEXT_SIZE - pat_len, pat_len);
        passed &= run_random_test(test_results, pattern, pat_len, T, "Patterns at the end", sigma);

        if (!passed)
            break;
    }

    free(pattern);
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

    int max_pat_len = MIN(TEST_PATTERN_MAX_LEN, TEST_SHORT_PAT_LEN);
    unsigned char *pattern = (unsigned char *)calloc(max_pat_len + 1, sizeof(unsigned char));

    // Test short patterns near end of text:
    for (int pat_len = 1; pat_len < max_pat_len; pat_len++)
    {
        memcpy(pattern, T + TEST_TEXT_SIZE - (pat_len * 2), pat_len);
        passed &= run_random_test(test_results, pattern, pat_len, T, "Patterns near end", sigma);

        if (!passed)
            break;
    }

    free(pattern);
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

    unsigned char *pattern = (unsigned char *)calloc(TEST_PATTERN_MAX_LEN + 1, sizeof(unsigned char));

    // Test consecutive pattern matches (modifies the text)
    int num_tests = test_results->opts->test_type == QUICK_TESTS ? TEST_QUICK_ITERATIONS : TEST_ITERATIONS;
    for (int num_to_test = 0; num_to_test < num_tests; num_to_test++)
    {
        int m = gen_consecutive_pattern_in_text(pattern, T);
        passed &= run_random_test(test_results, pattern, m, T, "Consecutive pattern", sigma);

        if (!passed)
            break;
    }

    free(pattern);
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

    unsigned char *pattern = (unsigned char *)calloc(TEST_PATTERN_MAX_LEN + 1, sizeof(unsigned char));

    // Test partial consecutive pattern (only a partial pattern after the first one, missing the first char of the pattern).
    int num_tests = test_results->opts->test_type == QUICK_TESTS ? TEST_QUICK_ITERATIONS : TEST_ITERATIONS;
    for (int num_to_test = 0; num_to_test < num_tests; num_to_test++)
    {
        int m = gen_partial_overlapping_pattern_in_text(pattern, T);
        passed &= run_random_test(test_results, pattern, m, T, "Partial consecutive pattern", sigma);

        if (!passed)
            break;
    }

    free(pattern);
    return passed;
}

/*
 * Find a random pattern, and copy it to the text so the pattern ends one char past the end of the actual text.
 * This helps to find algorithms that don't respect the text boundary correctly.
 * Returns true if passed.
 */
int test_pattern_past_end(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;

    unsigned char *pattern = (unsigned char *)calloc(TEST_PATTERN_MAX_LEN + 1, sizeof(unsigned char));

    // Test partial consecutive pattern (only a partial pattern after the first one, missing the first char of the pattern).
    int num_tests = test_results->opts->test_type == QUICK_TESTS ? TEST_QUICK_ITERATIONS : TEST_ITERATIONS;
    for (int num_to_test = 0; num_to_test < num_tests; num_to_test++)
    {
        int m = get_random_pattern_from_text(pattern, T);
        memcpy(T + TEST_TEXT_SIZE - m + 1, pattern, m);
        passed &= run_random_test(test_results, pattern, m, T, "Pattern past end", sigma);

        if (!passed)
            break;
    }

    free(pattern);
    return passed;
}


/*
 * Takes a pattern from the text, and copies everything but the first char into the first position of the text.
 */
int test_partial_pattern_at_start(test_results_info_t *test_results, unsigned char *T, int sigma)
{
    int passed = 1;

    if (TEST_TEXT_PRE_BUFFER > 0)  // If we have some room defined before the main text
    {
        unsigned char *pattern = (unsigned char *)calloc(TEST_PATTERN_MAX_LEN + 1, sizeof(unsigned char));

        // Test partial consecutive pattern (only a partial pattern after the first one, missing the first char of the pattern).
        int num_tests = test_results->opts->test_type == QUICK_TESTS ? TEST_QUICK_ITERATIONS : TEST_ITERATIONS;
        for (int num_to_test = 0; num_to_test < num_tests; num_to_test++)
        {
            int m = get_random_pattern_from_text(pattern, T);

            memcpy(T - 1, pattern, m);
            passed &= run_random_test(test_results, pattern, m, T, "Pattern before text start", sigma);

            if (!passed)
                break;
        }

        free(pattern);
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
 * Returns whether the tests passed.
 */
int run_random_tests(test_results_info_t *test_results, unsigned char *T)
{
    int passed = 1;

    int start_percent = 3;  // buffer overflow is 1, and fixed tests is 2 as a very rough estimate.
    int percent_done = start_percent;
    print_test_status(test_results, percent_done, "Randomised tests");

    // Always start with same random seed for each algorithm.
    // Ensures that different sets of algorithms will not affect the test results for each algorithm.
    srand(test_results->opts->random_seed);

    // Set the alphabet sizes to test for either quick or standard tests:
    int start_alphabet = test_results->opts->test_type == QUICK_TESTS ? 64 : 1;
    int end_alphabet   = test_results->opts->test_type == QUICK_TESTS ? 128 : 256;
    int range = end_alphabet - start_alphabet;
    int increment      = test_results->opts->test_type == QUICK_TESTS ? 4 : 1;

    // Run tests with random text of all alphabets.
    for (int sigma = end_alphabet; sigma >= start_alphabet; sigma -= increment)
    {
        double done = (double)(end_alphabet - sigma) / range;
        percent_done = update_random_test_percentage(test_results, done, start_percent, percent_done);

        gen_random_text(sigma, T, TEST_TEXT_SIZE);

        passed &= test_random_patterns(test_results, T, sigma);
        passed &= test_bad_first_char_patterns(test_results, T, sigma);
        passed &= test_patterns_at_start(test_results, T, sigma);
        passed &= test_patterns_near_start(test_results, T, sigma);
        passed &= test_patterns_at_end(test_results, T, sigma);
        passed &= test_patterns_near_end(test_results, T, sigma);
        passed &= test_consecutive_patterns(test_results, T, sigma);
        passed &= test_consecutive_partial_patterns(test_results, T, sigma);
        passed &= test_pattern_past_end(test_results, T, sigma);
        passed &= test_partial_pattern_at_start(test_results, T, sigma);

        if (!passed)
        {
            break; // don't test more alphabets if we have failures in this one.
        }
    }

    return passed;
}

/*
 * Tests an algorithm to detect if it writes to the text buffer, and if so, does it exceed the normal buffer limit.
 * Returns true (1) if the test passed, and (0) if there was a buffer overflow failure.
 */
int run_buffer_overflow_tests(test_results_info_t *test_results)
{
    print_test_status(test_results, 1, "Buffer overflow tests");
    int overflow_test_passed = 1;

    // Get the largest buffer size smart supports:
    int supported_buffer_size = get_text_buffer_size(TEST_TEXT_SIZE, TEST_PATTERN_MAX_LEN);

    // Allocate a larger buffer than normal, so we have some room to detect overflows without crashing ourselves...
    int buffer_size = supported_buffer_size * 2;
    unsigned char *search_data = (unsigned char *)malloc(sizeof(unsigned char) * buffer_size);
    unsigned char *copy_data = (unsigned char *)malloc(sizeof(unsigned char) * buffer_size);

    // Allocate heap memory for the pattern:
    unsigned char *pattern = (unsigned char *)calloc(TEST_PATTERN_MAX_LEN + 1, sizeof(unsigned char));

    // all ones in the pattern, all zeros in the search data.
    memset(pattern, 1, TEST_PATTERN_MAX_LEN + 1);
    memset(search_data, 0, buffer_size);

    // set random data past the search data, and take a copy of the whole search data buffer to check later.
    gen_random_text(256, search_data + TEST_TEXT_SIZE, buffer_size - TEST_TEXT_SIZE);
    memcpy(copy_data, search_data, buffer_size);

    // Test the algorithm.
    int test_count_passed = run_random_test(test_results, pattern, TEST_PATTERN_MAX_LEN, search_data, "Mismatched pattern", 256);

    // Test that the actual search text is not modified:
    for (int i = 0; i < TEST_TEXT_SIZE; i++)
    {
        if (copy_data[i] != search_data[i])
        {
            add_failure_message(test_results, "Overwrote the search text at position %s in a text of size %d",
                                i, TEST_TEXT_SIZE);
            overflow_test_passed = 0; // overwriting the text itself is a major failure.

            debug_search(test_results, pattern, TEST_PATTERN_MAX_LEN, search_data, TEST_TEXT_SIZE);
            break;
        }
    }

    // Test whether data after the supported buffer size has been modified.
    for (int i = buffer_size - 1; i >= supported_buffer_size; i--)
    {
        if (copy_data[i] != search_data[i])
        {
            add_failure_message(test_results, "Overwrote the buffer beyond the supported buffer size.");
            overflow_test_passed = 0; // overwriting text beyond the max buffer size supported is a major failure.

            debug_search(test_results, pattern, TEST_PATTERN_MAX_LEN, search_data, TEST_TEXT_SIZE);
            break;
       }
    }

    // the test_algo method will say it passed if it gets the correct count, but we don't care about whether the
    // search count is correct here; we're looking to see if it overflows the text buffer or modifies the text itself.
    if (test_count_passed)
    {
        test_results->num_passed--;  // adjust the passing test count down - we don't care about that pass.
    }

    // If we passed the actual overflow buffer test, add a passed result.
    if (overflow_test_passed)
    {
        test_results->num_passed++;
    }

    free(pattern);
    free(search_data);
    free(copy_data);

    return overflow_test_passed;
}

/*
 * Prints test failure result.
 */
void print_failure_result(test_results_info_t *test_results)
{
    if (test_results->num_tests == 0)
    {
        printf("\r\tTested  %-*s [--]      No tests executed.        (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
               test_results->num_passed, test_results->num_tests);
    }
    else if (test_results->num_passed == 0)
    {
        printf("\r\tTested  %-*s [FAIL]    All failed                (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
               test_results->num_passed, test_results->num_tests);
    }
    else if (test_results->num_passed < test_results->num_tests)
    {
        printf("\r\tTested  %-*s [FAIL]    Some failed               (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
               test_results->num_passed, test_results->num_tests);
    }
}

/*
 * Erases anything on the current line by issuing a CR, printing a line length of spaces and then issuing a CR again.
 */
void clear_line()
{
    printf("\r");
    for (int i = 0; i < MAX_LINE_LEN; i++) printf("%s", " ");
    printf("\r");
}

/*
 * Prints the final results of testing an algo.
 */
void print_test_results(test_results_info_t *test_results)
{
    // Output the summary line for the test.
    if (test_results->opts->fail_only)
    {
        if (test_results->num_failures > 0)
        {
            print_failure_result(test_results);
        }
        else
        {
            clear_line();
        }
    }
    else if (test_results->num_passed == test_results->num_tests)
    {
        printf("\r\tTested  %-*s [OK]      All passed                (%d/%d)    \n", ALGO_NAME_LEN, test_results->algo_name,
               test_results->num_passed, test_results->num_tests);
    }
    else
    {
        print_failure_result(test_results);
    }


    // Report failure messages, if any.
    for (int i = 0; i < test_results->num_failures; i++)
    {
        printf("\t        %-*s           %s\n", ALGO_NAME_LEN, test_results->algo_name, test_results->failure_messages[i]);
    }
}

/*
 * Finds any tests which failed that were previously flagged as passed in the tested algorithm set,
 * and adds their hashes to the failed_hashes string set.
 */
void build_failed_hashes( str_set_t *failed_hashes, const algo_info_t *algorithms,
                          const int *passed, tested_algo_info_t *tested_algorithms)
{
    str_set_init(failed_hashes);
    for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
    {
        if (!passed[algo_no] && algorithm_has_pass_record(algorithms, algo_no, tested_algorithms))
        {
            char hash_string[ALGO_HASH_LEN];
            snprintf(hash_string, ALGO_HASH_LEN, "%llu", algorithms->algo_hash_digest[algo_no]);
            str_set_add_copy(failed_hashes, hash_string);
        }
    }
}

/*
 * Returns true if the hash (field 1) is in the set of failed hashes.
 */
int line_contains_failed_hash(const char *line, str_set_t *failed_hashes)
{
    char hash_value[STR_BUF];
    if (get_tab_field(line, 1, hash_value, STR_BUF))
    {
        return str_set_contains(failed_hashes, hash_value);
    }

    return 0;
}

/*
 * Prints a status message if a line is being removed.
 */
void print_removing_algo(const char *line, int num_removed)
{
    char algo_name[STR_BUF];
    printf("%s", num_removed == 0 ? "\tRemoving previously passed tests for algorithms " : ", ");
    printf( "%s", get_tab_field(line, 0, algo_name, STR_BUF) ? algo_name : "{no algo name found}");
}

/*
 * Writes out a new tested_algos file, removing any lines that contain the failing hashes.
 * Returns the number of lines removed.
 */
int write_failing_test_results(const smart_config_t *smart_config, str_set_t *failed_hashes)
{
    int num_removed = 0;

    char tested_algo_filename[MAX_PATH_LENGTH];
    set_full_path_or_exit(tested_algo_filename, smart_config->smart_config_dir, TESTED_ALGOS_FILENAME);

    char tmp_file_name[MAX_PATH_LENGTH];
    set_filename_suffix_or_exit(tmp_file_name, tested_algo_filename, ".tmp");

    FILE *tested_algo_file = fopen(tested_algo_filename, "r");
    FILE *new_algo_file = fopen(tmp_file_name, "w");

    if (tested_algo_file != NULL && new_algo_file != NULL)
    {
        char * line = NULL;
        size_t len = 0;

        while ((getline(&line, &len, tested_algo_file)) != -1)
        {
            if (!line_contains_failed_hash(line, failed_hashes))
            {
                fprintf(new_algo_file, "%s", line);
            }
            else
            {
                print_removing_algo(line, num_removed++);
            }
        }

        if (num_removed > 0) printf(".\n");

        if (line != NULL) free(line);

        fclose(tested_algo_file);
        fclose(new_algo_file);

        rename(tmp_file_name, tested_algo_filename);
        remove(tmp_file_name);
    }
    else
    {
        warn("Could not open tested algorithm file to write results to at %s/%s", smart_config->smart_config_dir,
             TESTED_ALGOS_FILENAME);
    }

    return num_removed;
}

/*
 * Removes any test results that had previously passed but have now failed a further test.
 * Returns the number of passes removed.
 */
int remove_failing_test_results(const smart_config_t *smart_config, const algo_info_t *algorithms,
                                const int *passed, tested_algo_info_t *tested_algorithms)
{
    int num_removed = 0;
    str_set_t failed_hashes;
    build_failed_hashes(&failed_hashes, algorithms, passed, tested_algorithms);

    if (failed_hashes.size > 0)
        num_removed = write_failing_test_results(smart_config, &failed_hashes);

    str_set_free(&failed_hashes);

    return num_removed;
}

/*
 * Appends a single passing test result to the tested_algo file.
 */
void append_passing_test_result(const algo_info_t *algorithms, int algo_no, FILE *tested_algo_file, int num_updated)
{
    char time_now[26];
    set_time_string(time_now, 26, "%Y:%m:%d %H:%M:%S");

    char uppercase_algoname[ALGO_NAME_LEN];
    set_upper_case_algo_name(uppercase_algoname, algorithms->algo_names[algo_no]);

    fprintf(tested_algo_file, "%s\t%llu\t%s\tcommit:%s\tbuild time:%s\n",
            uppercase_algoname, algorithms->algo_hash_digest[algo_no], time_now, COMMIT, BUILD_TIME);

    const char *format = num_updated == 0 ? "\tAdded new pass results for %s" : ", %s";
    printf(format, uppercase_algoname);
}

/*
 * Records test successes in the tested_algos file in the config directory, if they do not already exist.
 * Returns the number of new test records appended.
 */
int append_passing_test_results(const smart_config_t *smart_config, const algo_info_t *algorithms,
                                const int *passed, tested_algo_info_t *tested_algorithms)
{
    char fullpath[MAX_PATH_LENGTH];
    set_full_path_or_exit(fullpath, smart_config->smart_config_dir, TESTED_ALGOS_FILENAME);

    int num_updated = 0;

    FILE *tested_algo_file = fopen(fullpath, "a");
    if (tested_algo_file != NULL)
    {
        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            if (passed[algo_no] &&
               !algorithm_has_pass_record(algorithms, algo_no, tested_algorithms))
            {
                append_passing_test_result(algorithms, algo_no, tested_algo_file, num_updated++);
            }
        }

        printf("%s", num_updated > 0 ? ".\n" : "\tNo new passing test results were recorded.\n");
        fclose(tested_algo_file);
    }
    else
    {
        warn("Could not open tested algorithm file to write results to at %s/%s", smart_config->smart_config_dir,
             TESTED_ALGOS_FILENAME);
    }

    return num_updated;
}

/*
 * Records the test results in the tested_algos file, if the test is set to update after running full tests.
 *
 *  - adds any newly passing algorithms to the tested_algos file.
 *  - removes any previously passed algorithms that have now failed the current tests.
 *
 *  Since tests are randomized, a previously passing algorithm can fail on later tests.
 */
void record_test_results(const smart_config_t *smart_config, const test_command_opts_t *opts, const algo_info_t *algorithms, int *pass_results)
{
    if (opts->test_type == FULL_TEST_AND_UPDATE)
    {
        tested_algo_info_t tested_algorithms;
        init_and_load_tested_algorithms(smart_config, &tested_algorithms);

        remove_failing_test_results(smart_config, algorithms, pass_results, &tested_algorithms);
        append_passing_test_results(smart_config, algorithms, pass_results, &tested_algorithms);

        free_tested_algo_info(&tested_algorithms);
    }
}

/*
 * Runs all the tests for the algorithm defined in test_results, using text buffer T for random tests.
 * Returns whether the algo passed.
 */
int run_all_tests(test_results_info_t *test_results, unsigned char *T)
{
    int passed = 1;

    if ((passed &= run_buffer_overflow_tests(test_results)))
    {
        passed &= run_fixed_tests(test_results);
        passed &= run_random_tests(test_results, T);
    }

    return passed;
}

/*
 * Prints a short summary of algorithms tested and failed.
 */
void print_test_summary(const algo_info_t *algorithms, int algos_passed)
{
    info("");
    int num_failures =  algorithms->num_algos - algos_passed;
    const char *plural_algo = algorithms->num_algos == 1 ? "" : "s";
    const char *plural_failure = num_failures == 1 ? "" : "s";
    info("Tested %d algorithm%s with %d failure%s.", algorithms->num_algos, plural_algo, num_failures, plural_failure);
}

/*
 * Tests all the algorithms with buffer overflow, fixed and random tests.
 *
 * Different algorithms will end up running different numbers of tests, since some algorithms cannot search for
 * some patterns, e.g. an algorithm using qgrams of length 4 cannot search for patterns less than 4 in length.
 *
 * The number of tests which actually ran, and the number of passing tests is recorded for each algorithm in the results struct.
 */
void test_algos(const smart_config_t *smart_config, const test_command_opts_t *opts, const algo_info_t *algorithms)
{
    if (algorithms->num_algos > 0)
    {
        int pass_results[algorithms->num_algos];
        unsigned long buffer_size = (TEST_TEXT_PRE_BUFFER * sizeof(unsigned char)) + get_text_buffer_size(TEST_TEXT_SIZE, TEST_PATTERN_MAX_LEN);
        unsigned char *T = (unsigned char *)malloc(buffer_size);

        int algos_passed = 0;
        test_results_info_t test_results;
        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            memset(T, 0, buffer_size); // zero out memory, including pre-buffer before each algo run.
            init_test_results(&test_results, opts, algorithms->algo_names[algo_no], algorithms->algo_functions[algo_no]);

            print_test_status(&test_results, 0, "");

            pass_results[algo_no] = run_all_tests(&test_results, T + TEST_TEXT_PRE_BUFFER);

            algos_passed += pass_results[algo_no];

            print_test_results(&test_results);
        }

        print_test_summary(algorithms, algos_passed);
        record_test_results(smart_config, opts, algorithms, pass_results);

        free(T);
    }
    else
    {
        warn("No algorithms were provided to test.");
    }

    info("");
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
        // This is basically the same as the load algos for run now.  Refactor to be a single method?
        // method in algorithms.h - get_valid_algorithm_names(algorithms, algo source, names, num names, smart config).
        read_algo_names_from_file(smart_config, algorithms, SELECTED_ALGOS_FILENAME);
        merge_regex_algos(smart_config, opts, algorithms);
        break;
    }
    case NAMED_SET_ALGOS:
    {
        char set_filename[STR_BUF];
        snprintf(set_filename, STR_BUF, "%s.algos", opts->named_set); //TODO: should we put the file name in here like the run command does?  make parser do the work.
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
 * Prints info and warning messages about the test options selected.
 */
void print_test_option_messages(const test_command_opts_t *opts)
{
    if (opts->test_type == QUICK_TESTS)
    {
        warn("Running quick tests - these results are not as reliable but give faster feedback.\n");
    }

    if (opts->pattern_info.pattern_min_len > 0)
    {
        info("Testing random patterns with pattern lengths from %d to %d, incrementing by %c %d.\n",
             opts->pattern_info.pattern_min_len, opts->pattern_info.pattern_max_len,
             opts->pattern_info.increment_operator, opts->pattern_info.increment_by);
    }
}


/*
 * Runs tests against the algorithms selected by the test command.
 */
void run_tests(const smart_config_t *smart_config, const test_command_opts_t *opts)
{
    // Get algos to test.
    algo_info_t algorithms;
    get_algonames_to_test(&algorithms, opts, smart_config);
    load_algo_shared_libraries(smart_config, &algorithms);

    // Print test info:
    print_algorithms_as_list("\tTesting ", &algorithms);
    printf("\n");
    print_test_option_messages(opts);
    char time_format[26];
    set_time_string(time_format, 26, "%Y:%m:%d %H:%M:%S");
    info("Algorithm correctness tests started at %s", time_format);

    // Test
    test_algos(smart_config, opts, &algorithms);

    // Finish testing.
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


