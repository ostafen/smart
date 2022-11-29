
#ifndef SMART_DEFINES_H
#define SMART_DEFINES_H

#define SIGMA 256                         // constant alphabet size
#define MEGA_BYTE (1024 * 1024)           // constant for 1 MB size

#define MAX_SEARCH_PATHS 32               // maximum search paths for data or algo_names set via environment variable.
#define MAX_SELECT_ALGOS 2048             // maximum number of algorithms which can be selected for benchmarking.

#define TEXT_SIZE_DEFAULT 1048576         // default size of text buffer for benchmarking
#define TEXT_SIZE_PADDING 256             // extra memory allocated to end of text buffer for safety in case algorithms modify too much text.
#define MAX_FILES 128                     // max number of different files to load to fill the benchmark text buffer.

#define PATTERN_MIN_LEN_DEFAULT 2         // default minimum size of pattern to test.
#define PATTERN_MAX_LEN_DEFAULT 4096      // default maximum size of pattern to test.

#define TEST_TEXT_SIZE 1048576            // size of text buffer for testing.
#define TEST_PATTERN_MAX_LEN 4096         // max length of random patterns to use when testing.

#define MAX_LINE_LEN 128                  // max length of line to output to console.
#define MAX_DATA_SOURCES 32               // max number of algorithm name/regexes to specify on command line.

#define NUM_PATTERNS_MAX 100              // maximum number of different pattern lengths to benchmark at one time.
#define NUM_RUNS_DEFAULT 500              // default number of patterns of a given size to benchmark with.
#define TIME_LIMIT_MILLIS_DEFAULT 300     // default time limit in milliseconds for a search to pass benchmarking.

#define CPU_PIN_DEFAULT "last"            // default CPU pinning - can be [off | last | {digit}]

#define STR_BUF 256                       // general strings with a bit of space to spare - parameters, filenames, etc.
#define MAX_PATH_LENGTH 2048              // Maximum file path length.
#define ALGO_NAME_LEN 32                  // Maximum length of an algorithm name.

#endif //SMART_DEFINES_H
