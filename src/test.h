#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "config.h"
#include "commands.h"
#include "algorithms.h"

typedef struct test_results_info
{
    const test_command_opts_t  *opts;
    const char *algo_name;
    search_function *search_func;
    int num_tests;
    int num_passed;
    int last_expected_count;
    int last_actual_count;
} test_results_info_t;

/*
 // 12) search for rand in rand
 for (h = 0; h < 10; h++)
     T[h] = rand() % 128;
 for (h = 0; h < 4; h++)
     P[h] = T[h];
 T[YSIZE] = P[4] = '\0';
 if (!attempt(&rip, count, P, 4, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 12))
     exit(1);

 // 13) search for rand in rand
 for (h = 0; h < 10; h++)
     T[h] = rand() % 128;
 for (h = 0; h < 4; h++)
     P[h] = T[h];
 T[10] = P[4] = '\0';
 if (!attempt(&rip, count, P, 4, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 13))
     exit(1);

 // 14) search for rand in rand
 for (h = 0; h < 64; h++)
     T[h] = rand() % 128;
 for (h = 0; h < 40; h++)
     P[h] = T[h];
 T[64] = P[40] = '\0';
 if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 14))
     exit(1);

 // 15) search for rand in rand
 for (h = 0; h < 64; h++)
     T[h] = rand() % 128;
 for (h = 0; h < 40; h++)
     P[h] = T[h];
 T[64] = P[40] = '\0';
 if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 15))
     exit(1);
*/

/*
// 16) search for rand in rand
for (h = 0; h < 64; h++)
    T[h] = 'a';
for (h = 0; h < 40; h++)
    P[h] = 'a';
T[64] = P[40] = '\0';
if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 16))
    exit(1);

// 17) search for rand in rand
for (h = 0; h < 64; h += 2)
    T[h] = 'a';
for (h = 1; h < 64; h += 2)
    T[h] = 'b';
for (h = 0; h < 40; h += 2)
    P[h] = 'a';
for (h = 1; h < 40; h += 2)
    P[h] = 'b';
T[64] = P[40] = '\0';
if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 17))
    exit(1);

// 18) search for rand in rand
for (h = 0; h < 64; h += 2)
    T[h] = 'a';
for (h = 1; h < 64; h += 2)
    T[h] = 'b';
for (h = 0; h < 40; h += 2)
    P[h] = 'a';
for (h = 1; h < 40; h += 2)
    P[h] = 'b';
P[39] = 'c';
T[64] = P[40] = '\0';
if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 18))
    exit(1);

*/

/* the brute force algorithm used for comparing occurrences */
int brute_force_search(const unsigned char *x, int m, const unsigned char *y, int n)
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

int test_algo(unsigned char *pattern, int m, unsigned char *data, int n, test_results_info_t *test_results)
{
    int test_passed = 1; // we assume passed and fail it explicitly.

    double search_time = 0;
    double pre_time = 0;

    // Allocate a bigger buffer than the search data, for algorithms that write sentinel values at the end of the search data.
    unsigned char *search_data = malloc(sizeof(unsigned char) * n + (2 * m));
    memcpy(search_data, data, n);

    // Get the result for the algorithm being tested:
    test_results->last_actual_count = test_results->search_func(pattern, m, search_data, n, &search_time, &pre_time);

    // if we have a count, add to test results.  negative result means algo can't search that pattern, so we ignore the test.
    if (test_results->last_actual_count >= 0)
    {
        test_results->num_tests += 1;
        test_results->last_expected_count = brute_force_search( pattern, m, data, n);
        if (test_results->last_actual_count == test_results->last_expected_count)
        {
            test_results->num_passed += 1;
        }
        else
        {
            test_passed = 0; // test failed.
        }
    }

    free(search_data);

    return test_passed;
}

void test_fixed_string(char *pattern, char *text, test_results_info_t *test_results)
{
    int m = strlen(pattern);
    int n = strlen(text);

    //Implicit cast because C strings are char * but search function is defined with unsigned char *.
    if (!test_algo(pattern, m, text, n, test_results) && test_results->opts->verbose)
    {
        info("\t%s found %d of %d occurrences, searching '%s' in '%s'", test_results->algo_name,
             test_results->last_actual_count, test_results->last_expected_count, pattern, text);
    }
}

void test_message(const char *description, const test_results_info_t *test_results)
{
    if (test_results->opts->verbose)
    {
        info("Testing %-*s         %s", ALGO_NAME_LEN, test_results->algo_name, description);
        fflush(stdout);
    }
}


/*
 * Short fixed length string tests.
 */
int run_fixed_tests(test_results_info_t *test_results)
{
    test_message("Fixed String Tests", test_results);

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

int get_random_pattern_from_text(unsigned char pattern[TEST_PATTERN_MAX_LEN], unsigned char *T)
{
    int pat_len = (rand() % (TEST_PATTERN_MAX_LEN - 1)) + 1;
    pat_len = pat_len < 1 ? 1 : pat_len;
    int start = rand() % (TEST_PATTERN_MAX_LEN - pat_len);
    memcpy(pattern + start, T, pat_len);
    return pat_len;
}

int run_random_test(test_results_info_t *test_results, unsigned char *pattern, int m, unsigned char *T,
                     const char *test_description, int sigma)
{
    int passed = test_algo(pattern, m, T, TEST_TEXT_SIZE, test_results);
    if (!passed && test_results->opts->verbose)
    {
        info("\t%s failed on %s tests in random text of alphabet %d, failed with pattern length %d, random seed: %ld",
             test_results->algo_name, test_description, sigma, m, test_results->opts->random_seed);
    }
    return passed;
}

void run_random_tests(test_results_info_t *test_results, unsigned char *T)
{
    test_message("Random String Tests", test_results);

    srand(test_results->opts->random_seed); // ensure we always start with same random seed for these tests.
    for (int sigma = 1; sigma <= 256; sigma++)
    {
        gen_random_text(sigma, T, TEST_TEXT_SIZE);
        unsigned char pattern[TEST_PATTERN_MAX_LEN];

        int passed = 1;

        // Test random patterns extracted from the text:
        for (int test_no = 1; test_no <= NUM_RANDOM_PATTERN_TESTS; test_no++)
        {
            // Test pattern which exists in the text:
            int m = get_random_pattern_from_text(pattern, T);
            passed &= run_random_test(test_results, pattern, m, T, "random positions", sigma);

            // Test pattern which may not exist in the text but only modifies the first character:
            pattern[0] ^= pattern[0];
            passed &= run_random_test(test_results, pattern, m, T, "first char modified", sigma);
        }

        // Test short patterns from start of text:
        for (int pat_len = 1; pat_len < 16; pat_len++)
        {
            memcpy(pattern, T, pat_len);
            passed &= run_random_test(test_results, pattern, pat_len, T, "patterns at the start", sigma);
        }

        // Test short patterns at end of text:
        for (int pat_len = 1; pat_len < 16; pat_len++)
        {
            memcpy(pattern, T + TEST_TEXT_SIZE - pat_len, pat_len);
            passed &= run_random_test(test_results, pattern, pat_len, T, "patterns at the end", sigma);
        }

        if (!passed)
        {
            break; // don't test more alphabets if we have failures in this one.
        }
    }
}

void init_test_results(test_results_info_t *results, const test_command_opts_t *opts,
                       const char *algo_name, search_function *search_func)
{
    results->algo_name = algo_name;
    results->search_func = search_func;
    results->num_tests = 0;
    results->num_passed = 0;
    results->opts = opts;
}

void print_test_results(test_results_info_t *test_results)
{
    if (test_results->num_tests == 0)
    {
        info("Tested  %-*s [--]    No tests executed.", ALGO_NAME_LEN, test_results->algo_name);
    }
    else if (test_results->num_passed == test_results->num_tests)
    {
        info("Tested  %-*s [OK]    All passed                (%d/%d)", ALGO_NAME_LEN, test_results->algo_name,
             test_results->num_passed, test_results->num_tests);
    }
    else if (test_results->num_passed == 0)
    {
        info("Tested  %-*s [ERROR] None passed               (%d/%d)", ALGO_NAME_LEN, test_results->algo_name,
             test_results->num_passed, test_results->num_tests);
    }
    else
    {
        info("Tested  %-*s [ERROR] Some failed               (%d/%d)", ALGO_NAME_LEN, test_results->algo_name,
             test_results->num_passed, test_results->num_tests);
    }
    if (test_results->opts->verbose)
    {
        printf("\n"); // split up the results a bit, gets noisy when verbose.
    }
}

void test_algos(const test_command_opts_t *opts, unsigned char *T, const algo_info_t *algorithms)
{
    if (algorithms->num_algos > 0)
    {
        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            test_results_info_t test_results;
            init_test_results(&test_results, opts, algorithms->algo_names[algo_no], algorithms->algo_functions[algo_no]);
            run_fixed_tests(&test_results);
            run_random_tests(&test_results, T);
            print_test_results(&test_results);
        }
    }
    else
    {
        warn("No algorithms were provided to test.\n");
    }
}

void merge_regex_algos(const smart_config_t *smart_config, const test_command_opts_t *opts, algo_info_t *algorithms)
{
    if (opts->num_algo_names > 0) {
        algo_info_t regex_algos;
        get_all_algo_names(smart_config, &regex_algos);
        filter_out_names_not_matching_regexes(&regex_algos, NULL, opts->algo_names, opts->num_algo_names);
        merge_algorithms(algorithms, &regex_algos, NULL);
    }
}

void get_algonames_to_test(algo_info_t *algorithms, const test_command_opts_t *opts, const smart_config_t *smart_config)
{
    algorithms->num_algos = 0;
    switch (opts->algo_source)
    {
        case ALGO_NAMES:
        {
            get_all_algo_names(smart_config, algorithms);
            filter_out_names_not_matching_regexes(algorithms, NULL, opts->algo_names, opts->num_algo_names);
            break;
        }
        case SELECTED:
        {
            read_algo_names_from_file(smart_config, algorithms, SELECTED_ALGOS_FILENAME);
            merge_regex_algos(smart_config, opts, algorithms);
            break;
        }
        case NAMED_SET:
        {
            char set_filename[STR_BUF];
            snprintf(set_filename, STR_BUF, "%s.algos", opts->named_set);
            read_algo_names_from_file(smart_config, algorithms, set_filename);
            merge_regex_algos(smart_config, opts, algorithms);
            break;
        }
        case ALL:
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
    // Load the algorithms to test with:
    algo_info_t algorithms;
    get_algonames_to_test(&algorithms, opts, smart_config);
    load_algo_shared_libraries(smart_config, &algorithms);

    // Allocate buffer to search in - tests will fill the buffer with different things for each test.
    unsigned char *T = (unsigned char *)malloc(sizeof(unsigned char) * (TEST_TEXT_SIZE + TEST_PATTERN_MAX_LEN + TEXT_SIZE_PADDING));

    // Test the algorithms:
    print_time_message("Algorithm correctness tests started at:");
    test_algos(opts, T, &algorithms);

    // Free memory and unload search algorithms.
    free(T);
    unload_algos(&algorithms);

    print_time_message("Algorithm correctness tests finished at:");
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
/*
int main(int argc, char *argv[])
{

	// begin testing
	int rip = 0;
	int alpha, k, h, m, occur1, occur2, test = 1;


	// 1) search for "a" in "aaaaaaaaaa"
	strcpy((char *)P, "a");
	strcpy((char *)T, "aaaaaaaaaa");
	if (!attempt(&rip, count, P, 1, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 1))
		exit(1);

	// 2) search for "aa" in "aaaaaaaaaa"
	strcpy((char *)P, "aa");
	strcpy((char *)T, "aaaaaaaaaa");
	if (!attempt(&rip, count, P, 2, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 2))
		exit(1);

	// 3) search for "aaaaaaaaaa" in "aaaaaaaaaa"
	strcpy((char *)P, "aaaaaaaaaa");
	strcpy((char *)T, "aaaaaaaaaa");
	if (!attempt(&rip, count, P, 10, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 3))
		exit(1);

	// 4) search for "b" in "aaaaaaaaaa"
	strcpy((char *)P, "b");
	strcpy((char *)T, "aaaaaaaaaa");
	if (!attempt(&rip, count, P, 1, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 4))
		exit(1);

	// 5) search for "abab" in "ababababab"
	strcpy((char *)P, "ab");
	strcpy((char *)T, "ababababab");
	if (!attempt(&rip, count, P, 2, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 5))
		exit(1);

	// 6) search for "a" in "ababababab"
	strcpy((char *)P, "a");
	strcpy((char *)T, "ababababab");
	if (!attempt(&rip, count, P, 1, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 6))
		exit(1);

	// 7) search for "aba" in "ababababab"
	strcpy((char *)P, "aba");
	strcpy((char *)T, "ababababab");
	if (!attempt(&rip, count, P, 3, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 7))
		exit(1);

	// 8) search for "abc" in "ababababab"
	strcpy((char *)P, "abc");
	strcpy((char *)T, "ababababab");
	if (!attempt(&rip, count, P, 3, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 8))
		exit(1);

	// 9) search for "ba" in "ababababab"
	strcpy((char *)P, "ba");
	strcpy((char *)T, "ababababab");
	if (!attempt(&rip, count, P, 2, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 8))
		exit(1);

	// 10) search for "babbbbb" in "ababababab"
	strcpy((char *)P, "babbbbb");
	strcpy((char *)T, "ababababab");
	if (!attempt(&rip, count, P, 7, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 10))
		exit(1);

	// 11) search for "bcdefg" in "bcdefghilm"
	strcpy((char *)P, "bcdefg");
	strcpy((char *)T, "bcdefghilm");
	if (!attempt(&rip, count, P, 6, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 11))
		exit(1);

	// 12) search for rand in rand
	for (h = 0; h < 10; h++)
		T[h] = rand() % 128;
	for (h = 0; h < 4; h++)
		P[h] = T[h];
	T[YSIZE] = P[4] = '\0';
	if (!attempt(&rip, count, P, 4, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 12))
		exit(1);

	// 13) search for rand in rand
	for (h = 0; h < 10; h++)
		T[h] = rand() % 128;
	for (h = 0; h < 4; h++)
		P[h] = T[h];
	T[10] = P[4] = '\0';
	if (!attempt(&rip, count, P, 4, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 13))
		exit(1);

	// 14) search for rand in rand
	for (h = 0; h < 64; h++)
		T[h] = rand() % 128;
	for (h = 0; h < 40; h++)
		P[h] = T[h];
	T[64] = P[40] = '\0';
	if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 14))
		exit(1);

	// 15) search for rand in rand
	for (h = 0; h < 64; h++)
		T[h] = rand() % 128;
	for (h = 0; h < 40; h++)
		P[h] = T[h];
	T[64] = P[40] = '\0';
	if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 15))
		exit(1);

	// 16) search for rand in rand
	for (h = 0; h < 64; h++)
		T[h] = 'a';
	for (h = 0; h < 40; h++)
		P[h] = 'a';
	T[64] = P[40] = '\0';
	if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 16))
		exit(1);

	// 17) search for rand in rand
	for (h = 0; h < 64; h += 2)
		T[h] = 'a';
	for (h = 1; h < 64; h += 2)
		T[h] = 'b';
	for (h = 0; h < 40; h += 2)
		P[h] = 'a';
	for (h = 1; h < 40; h += 2)
		P[h] = 'b';
	T[64] = P[40] = '\0';
	if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 17))
		exit(1);

	// 18) search for rand in rand
	for (h = 0; h < 64; h += 2)
		T[h] = 'a';
	for (h = 1; h < 64; h += 2)
		T[h] = 'b';
	for (h = 0; h < 40; h += 2)
		P[h] = 'a';
	for (h = 1; h < 40; h += 2)
		P[h] = 'b';
	P[39] = 'c';
	T[64] = P[40] = '\0';
	if (!attempt(&rip, count, P, 40, T, 64, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 18))
		exit(1);

	// 19) search for "babbbbb" in "abababbbbb"
	strcpy((char *)P, "babbbbb");
	strcpy((char *)T, "abababbbbb");
	if (!attempt(&rip, count, P, 7, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 19))
		exit(1);

	// 20) search for "bababb" in "abababbbbb"
	strcpy((char *)P, "bababb");
	strcpy((char *)T, "abababbbbb");
	if (!attempt(&rip, count, P, 6, T, 10, algoname, pkey, tkey, rkey, ekey, prekey, alpha, parameter, 20))
		exit(1);

	if (!VERBOSE)
		printf("\n\tWell done! Test passed successfully\n\n");

	return 0;
}

 */
