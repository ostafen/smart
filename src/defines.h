
#ifndef SMART_DEFINES_H
#define SMART_DEFINES_H

/*
 * Basic constants.
 */
#define SIGMA 256                         // constant alphabet size
#define MEGA_BYTE (1024 * 1024)           // constant for 1 MB size

/*
 * Path and algorithm string length limits.
 */
#define MAX_PATH_LENGTH 2048              // Maximum file path length.
#define ALGO_NAME_LEN 33                  // Maximum length of an algorithm name string + 1 for null terminator char.

/*
 * Default file and directory names.
 */
#define SMART_DATA_DIR_DEFAULT "data"
#define SMART_ALGO_DIR_DEFAULT "algos"
#define SMART_RESULTS_PATH_DEFAULT "results"
#define SMART_CONFIG_PATH_DEFAULT "config"
#define SELECTED_ALGOS_FILENAME "selected_algos"
#define ALGO_FILENAME_SUFFIX ".algos"

/*
 * Environment variable names to configure directories and search paths.
 */
#define SMART_DATA_DIR_ENV "SMART_DATA_DIR"
#define SMART_ALGO_DIR_ENV "SMART_ALGO_DIR"
#define SMART_RESULTS_DIR_ENV "SMART_RESULTS_DIR"
#define SMART_CONFIG_DIR_ENV "SMART_CONFIG_DIR"
#define SMART_DATA_SEARCH_PATHS_ENV "SMART_DATA_SEARCH_PATHS"
#define SMART_ALGO_SEARCH_PATHS_ENV "SMART_ALGO_SEARCH_PATHS"
#define SEARCH_PATH_DELIMITER ":"                              // delimiter used to separate search paths specified in environment variables.

/*
 * Maximum algorithms and data sources which can be specified.
 */
#define MAX_DATA_SOURCES 32               // max number of algorithm name/regexes to specify on command line.
#define MAX_SEARCH_PATHS 32               // maximum search paths for data or algo_names set via environment variable.
#define MAX_SELECT_ALGOS 2048             // maximum number of algorithms which can be selected for benchmarking.
#define MAX_DATA_FILES 128                // max number of different files to load to fill the benchmark text buffer.

/*
 * Benchmarking defaults
 */
#define TEXT_SIZE_DEFAULT 1048576         // default size of text buffer for benchmarking
#define TEXT_SIZE_PADDING 256             // extra memory allocated to end of text buffer for safety in case algorithms modify too much text.
#define PATTERN_MIN_LEN_DEFAULT 2         // default minimum size of pattern to benchmark.
#define PATTERN_MAX_LEN_DEFAULT 4096      // default maximum size of pattern to benchmark.
#define NUM_PATTERNS_MAX 100              // maximum number of different pattern lengths to benchmark at one time.
#define NUM_RUNS_DEFAULT 500              // default number of patterns of a given size to benchmark with.
#define TIME_LIMIT_MILLIS_DEFAULT 300     // default time limit in milliseconds for a search to pass benchmarking.
#define CPU_PIN_DEFAULT PIN_LAST_CPU      // default CPU pinning - can be [off | last | {digit}]

/*
 * Test defaults
 */
#define TEST_TEXT_SIZE 1048576            // size of text buffer for testing.
#define TEST_PATTERN_MAX_LEN 4096         // max length of random patterns to use when testing.

/*
 * Console output formatting defines.
 */
#define COL_WIDTH 32                              // width of column to output name value pairs with.
#define BENCHMARK_HEADER_LEN (ALGO_NAME_LEN + 4)  // Length of header containing algorithm name and dots when benchmarking.

/*
 * Miscellaneous string defines.
 */
#define MAX_LINE_LEN 128                  // max length of line to output to console.
#define STR_BUF 256                       // general strings with a bit of space to spare - parameters, filenames, etc.
#define STR_END_CHAR '\0'                 // zero char to terminate strings.

#endif //SMART_DEFINES_H
