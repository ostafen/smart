
#ifndef SMART_DEFINES_H
#define SMART_DEFINES_H

/*
 * Useful function defines.
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define WITHIN(v, l, u) (MAX((l), MIN((u), (v))))  // Returns a value between a lower bound l and upper bound u inclusive, given value v.

/*
 * Basic constants.
 */
#define SIGMA 256                         // constant alphabet size
#define MEGA_BYTE (1024 * 1024)           // constant for 1 MB size
#define GIGA_BYTE (MEGA_BYTE * 1024)      // constant for 1 Gb
#define TRUE 1                            // value of true
#define FALSE 0                           // value of false

/*
 * Search return codes
 */
#define INFO_CANNOT_SEARCH (-1)           // return code for algorithms that cannot search for a given pattern in a text.
#define ERROR_SEARCHING (-2)              // return code for algorithms that encounter an error while pre-processing or searching.

/*
 * Limits.
 */
#define MAX_PATH_LENGTH 2048              // Maximum file path length.
#define ALGO_NAME_LEN 25                  // Maximum length of an algorithm name string + 1 for null terminator char.
#define ALGO_HASH_LEN 22                  // Max length of an algorithm test hash.
#define MAX_DATA_SOURCES 32               // max number of algorithm name/regexes to specify on command line.
#define MAX_SEARCH_PATHS 32               // maximum search paths for data or algo_names set via environment variable.
#define MAX_SELECT_ALGOS 2048             // maximum number of algorithms which can be selected for benchmarking.
#define MAX_DATA_FILES 128                // max number of different files to load to fill the benchmark text buffer.
#define NUM_PATTERNS_MAX 100              // maximum number of different pattern lengths to benchmark at one time.
#define ALGO_REGEX_LEN 64                 // max length of algorithm regex from command line.
#define SMALL_LINE_LEN 128                // max length of line to output to console when formatting columns and tables.
#define MAX_LINE_LEN 256                  // max length of line to output to console.
#define MAX_LINES 5                       // max number of lines that can be output to the console in one go.
#define MAX_OUTPUT_LEN MAX_LINE_LEN * MAX_LINES // max output length when writing multiple lines
#define STR_BUF 256                       // general strings with a bit of space to spare - parameters, filenames, etc.
#define NUM_PATTERNS_AT_END_OF_TEXT 2     // Number of pattern-lengths to add to the text buffer so algorithms that write
                                          // to the end of the text (e.g. sentinel guard) do not cause buffer overflows.
#define TEXT_SIZE_PADDING 256             // extra memory allocated to end of text buffer for safety in case algorithms modify too much text.

/*
 * Default file and directory names.
 */
#define SMART_DATA_DIR_DEFAULT "data"
#define SMART_ALGO_DIR_DEFAULT "algos"
#define SMART_RESULTS_PATH_DEFAULT "results"
#define SMART_CONFIG_PATH_DEFAULT "config"
#define SELECTED_ALGOS_FILENAME "selected_algos"
#define TESTED_ALGOS_FILENAME "tested_algos"
#define ALGO_FILENAME_SUFFIX ".algos"
#define STATS_FILENAME_PREFIX "stat_"

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
 * Pattern length increment control
 */
#define INCREMENT_MULTIPLY_OPERATOR '*'   // operator for multiply (default)
#define INCREMENT_ADD_OPERATOR '+'        // operator for add.
#define INCREMENT_BY 2                    // default pattern increment is to multiply by 2

/*
 * Benchmarking defaults
 */
#define TEXT_SIZE_DEFAULT MEGA_BYTE       // default size of text buffer for benchmarking
#define PATTERN_MIN_LEN_DEFAULT 2         // default minimum size of pattern to benchmark.
#define PATTERN_MAX_LEN_DEFAULT 4096      // default maximum size of pattern to benchmark.
#define NUM_RUNS_DEFAULT 500              // default number of patterns of a given size to benchmark with.
#define TIME_LIMIT_MILLIS_DEFAULT 300     // default time limit in milliseconds for a search to pass benchmarking.
#define CPU_PIN_DEFAULT PIN_LAST_CPU      // default CPU pinning - a value from the enum cpu_pin_type in commands.h.
#define DEFAULT_PRECISION 2               // default precision for outputting results - number of decimal points to round to.

/*
 * Test settings
 */
#define TEST_TEXT_SIZE 8192               // size of text buffer for testing.
#define TEST_TEXT_PRE_BUFFER 64           // space to allocate at the start of the test text buffer to prevent seg faults if algorithms read before start of text.
                                          // IMPORTANT: this value must be word-aligned.  Some search algorithms require the text to start at a word boundary (e.g. SSEF).
#define TEST_PATTERN_MIN_LEN 1            // min length of random pattern to use when testing.
#define TEST_PATTERN_MAX_LEN 2048         // max length of random patterns to use when testing.
#define MAX_FAILURE_MESSAGES 32           // max number of failure messages to record.
#define TEST_ITERATIONS 12                // number of iterations of each test for standard tests.
#define TEST_QUICK_ITERATIONS 2           // number of iterations of each test for quick tests.
#define TEST_SHORT_PAT_LEN 16             // max length X of 1..X short pattern lengths tested at end and start.

/*
 * Console output formatting defines.
 */
#define COL_WIDTH 32                              // width of column to output name value pairs with.
#define BENCHMARK_HEADER_LEN (ALGO_NAME_LEN + 16)  // Length of header containing algorithm name and dots when benchmarking.

/*
 * Miscellaneous string defines.
 */
#define STR_END_CHAR '\0'                 // zero char to terminate strings.
#define TIME_FORMAT "%Y:%m:%d %H:%M:%S"   // format for time strings.
#define TIME_FORMAT_STRLEN 26             // required length of time format string.

#endif //SMART_DEFINES_H
