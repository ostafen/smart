/*
 * SMART: string matching algorithms research tool.
 * Copyright (C) 2012  Simone Faro and Thierry Lecroq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * contact the authors at: faro@dmi.unict.it, thierry.lecroq@univ-rouen.fr
 * download the tool at: http://www.dmi.unict.it/~faro/smart/
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <regex.h>
#include <ctype.h>

#include "defines.h"

#define FNV_HASH_OFFSET_32 2166136261                      // 32 bit starting offset for FNV1a hash: see see http://www.isthe.com/chongo/tech/comp/fnv/
#define FNV_HASH_OFFSET_64 14695981039346656037LLU         // 64 bit starting offset for FNV1a hash: see see http://www.isthe.com/chongo/tech/comp/fnv/
#define FNV_HASH_32(v, p) (((p) ^ (v)) * 16777619)         // 32 bit FNV1a hash calculation: see http://www.isthe.com/chongo/tech/comp/fnv/
#define FNV_HASH_64(v, p) (((p) ^ (v)) * 1099511628211)    // 64 bit FNV1a hash calculation: see http://www.isthe.com/chongo/tech/comp/fnv/

static const char CASE_GAP = 'a' - 'A';

/*
 * Prints the smart logo.
 */
void print_logo()
{
    printf("                                \n");
    printf("	                          _   \n");
    printf("	 ___ _ __ ___   __ _ _ __| |_ \n");
    printf("	/ __|  _   _ \\ / _  |  __| __|\n");
    printf("	\\__ \\ | | | | | (_| | |  | |_ \n");
    printf("	|___/_| |_| |_|\\__,_|_|   \\__|\n");
    printf("	A String Matching Research Tool\n");
    printf("	by Simone Faro, Matt Palmer, Stefano Scafiti and Thierry Lecroq\n");
    printf("	Last Build Time: %s\n", BUILD_TIME);
    printf("	Commit: %s\n", COMMIT);
    printf("\n");
    printf("	If you use this tool in your research please cite the following paper:\n");
    printf("	| Simone Faro and Thierry Lecroq\n");
    printf("	| The Exact Online String Matching Problem: a Review of the Most Recent Results\n");
    printf("	| ACM Computing Surveys, Vol. 45(2): p.13 (2013)\n");
    printf(" ");
}

/*
 * Prints a formatted message prefixed by ERROR: and terminated with a new line.
 * The program then exits with return code 1.
 */
void error_and_exit(const char * format, ...)
{
    printf("\n\tERROR: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");

    exit(1);
}

/*
 * Prints a standard formatted warning message prefixed by WARNING: and terminated with a new line.
 */
void warn(const char * format, ...)
{
    printf("\tWARNING: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

/*
 * Prints a message to the smart output with standard formatting.
 * Messages are preceded by a tab, and finished with a newline.
 */
void info(const char * format, ...)
{
    printf("\t");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

/*
 * Adds the filename to the path and places the result in fullname, up to MAX_PATH_LENGTH chars.
 * Returns true (1) if it copied the path successfully (even empty), and false (0) if it could not copy the full path.
 */
int set_full_path(char fullname[MAX_PATH_LENGTH], const char *path, const char *filename)
{
    size_t len = strlen(path);
    if (len > 0)
    {
        const char *format_string = path[len - 1] == '/' ? "%s%s" : "%s/%s";
        return snprintf(fullname, MAX_PATH_LENGTH, format_string, path, filename) <= MAX_PATH_LENGTH;
    }

    fullname[0] = STR_END_CHAR; // full path is empty - but success.
    return 1;
}

/*
 * Builds the full path given a path and a filename.  It can handle paths terminated in a / and those without a /.
 * If the length of both exceeds the max path length, an error message is printed and the program exits.
 */
void set_full_path_or_exit(char fullname[MAX_PATH_LENGTH], const char *path, const char *filename)
{
    if (!set_full_path(fullname, path, filename))
    {
        error_and_exit("Full path exceeds max path length of %d\n%s\n%s\n",
                       MAX_PATH_LENGTH, path, filename);
    }
}

/*
 * Returns true if a string is empty.
 */
int is_empty_string(const char *string)
{
    return string[0] == STR_END_CHAR;
}

/*
 * Sets string to zero length by placing a terminator character as the first char of the string.
 */
void set_empty_string(char *string)
{
    string[0] = STR_END_CHAR;
}

/*
 * Adds a suffix to a filename, but exits with an error message if the length exceeds the max path length.
 */
void set_filename_suffix_or_exit(char fullname[MAX_PATH_LENGTH], const char *filename, const char *suffix)
{
    if (snprintf(fullname, MAX_PATH_LENGTH, "%s%s", filename, suffix) > MAX_PATH_LENGTH)
    {
        error_and_exit("Full path exceeds max path length of %d\n%s%s\n",
                       MAX_PATH_LENGTH, filename, suffix);
    }
}

/*
 * returns "true" if the value is not zero, and "false" if the value is zero.
 */
const char *true_false(int value)
{
    return value ? "true" : "false";
}

/*
 * Returns a rate in Gigabytes per second, given a time in milliseconds and the number of bytes scanned in that time.
 */
double GBs(double time_ms, int num_bytes)
{
    return (double) num_bytes / time_ms * 1000 / GIGA_BYTE;
}

/*
 * Copies the string in path into the to_path, up to the limit of MAX_PATH_LENGTH.
 * Returns true (1) if it copied the path successfully (even empty), and false (0) if it could not copy the full path.
 */
int copy_path(char to_path[MAX_PATH_LENGTH], const char *path)
{
    return snprintf(to_path, MAX_PATH_LENGTH, "%s", path) <= MAX_PATH_LENGTH;
}

/*
 * Copies the path into the to_path string, or exits if the path is too long to fit with an error message.
 */
void copy_path_or_exit(char to_path[MAX_PATH_LENGTH], const char *path)
{
    if (!copy_path(to_path, path))
    {
        error_and_exit("Full path exceeds max path length of %d\n%s\n",
                       MAX_PATH_LENGTH, path);
    }
}

/*
 * Sets one_folder_back to be the previous folder in the path supplied, or exits if the length
 * exceeds the maximum file path.
 */
void set_one_folder_back_or_exit(char one_folder_back[MAX_PATH_LENGTH], const char *path)
{
    copy_path_or_exit(one_folder_back, path);
    size_t len = strlen(one_folder_back);
    if (len > 0)
    {
        // If we terminated the path with a trailing /, get rid of it:
        if (one_folder_back[len - 1] == '/')
        {
            one_folder_back[len - 1] = STR_END_CHAR;
        }

        // If a slash looking back from the end of the string exists, set it to 0, terminating the string there.
        char *last_slash_ptr = strrchr(one_folder_back, '/');
        if (last_slash_ptr != NULL)
        {
            *last_slash_ptr = STR_END_CHAR;
        }
    }
}

/*
 * Sets the random seed for this run, and prints a message showing the seed value to the user.
 */
void set_random_seed(const long random_seed)
{
    srand(random_seed);
    info("\n\tSetting random seed to %ld.  Use -rs %ld if you need to rerun identically.\n",
         random_seed, random_seed);
}

/*
 * Compares a double with another double.
 */
static int double_compare(const void *a, const void *b)
{
    return (*(double*)a > *(double*)b) ? 1 : (*(double*)a < *(double*)b) ? -1 : 0;
}

/*
 * Compares a string with another string.
 */
static int str_compare(const void *str1, const void *str2)
{
    return strcmp(str1, str2);
}

/*
 * Compares a list of strings with s to see if any match.
 * Returns 0 if it finds a match to any of the list, or -1 if it doesn't.
 */
int strcmpany(const char *s, int n, ...)
{
    int result = -1;
    va_list ptr;

    va_start(ptr, n);
    for (int i = 0; i < n; i++)
    {
        const char *curr_str = va_arg(ptr, const char *);
        if (!strcmp(curr_str, s))
        {
            result = 0;
            break;
        }
    }
    va_end(ptr);

    return result;
}

/*
 * Returns 1 if the string has the suffix, and 0 if not.
 */
int has_suffix(const char *s, const char *suffix)
{
    return !strcmp(s + strlen(s) - strlen(suffix), suffix);
}

/*
 * Trims whitespace from the end of a string, modifies string memory in-place.
 */
int trim_str(char *s)
{
    size_t pos = strlen(s);
    while (pos > 0)
    {
        char c = s[pos - 1];
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t')
        {
            pos--;
        }
        else {
            break;
        }

    }
    s[pos] = STR_END_CHAR;
}

/* returns 1 if s is an integer number. 0 otherwise */
int is_int(const char *s)
{
    size_t len = strlen(s);
    size_t i;
    for (i = 0; i < len; i++)
        if (s[i] < '0' || s[i] > '9')
            return 0;
    return 1;
}

/*
 * Converts string s to lower case.
 */
char *str2lower(char *s)
{
    size_t len = strlen(s);
    size_t pos = 0;
    while (pos < len)
    {
        if (s[pos] >= 'A' && s[pos] <= 'Z')
            s[pos] = s[pos] + CASE_GAP;
        pos++;
    }
    return s;
}

/*
 * Converts string s to upper case.
 */
char *str2upper(char *s)
{
    size_t len = strlen(s);
    size_t pos = 0;
    while (pos < len)
    {
        if (s[pos] >= 'a' && s[pos] <= 'z')
            s[pos] = s[pos] - CASE_GAP;
        pos++;
    }
    return s;
}

/*
 * Sets the upper_case algo name to an upper case version of the source.
 */
void set_upper_case_algo_name(char upper_case[ALGO_NAME_LEN], const char source[ALGO_NAME_LEN])
{
    snprintf(upper_case, ALGO_NAME_LEN, "%s", source);
    str2upper(upper_case);
}

/*
 * Sets the lower case algo name to a lower case version of the source.
 */
void set_lower_case_algo_name(char lower_case[ALGO_NAME_LEN], const char source[ALGO_NAME_LEN])
{
    snprintf(lower_case, ALGO_NAME_LEN, "%s", source);
    str2lower(lower_case);
}

/*
 * Sets a path to either an environment variable if set, or the default path if it isn't.
 */
void set_env_var_or_default(char path_to_set[MAX_PATH_LENGTH], const char *env_var_name, char default_path[MAX_PATH_LENGTH])
{
    const char * env_var = getenv(env_var_name);
    if (env_var == NULL)
    {
        copy_path_or_exit(path_to_set, default_path);
    }
    else
    {
        copy_path_or_exit(path_to_set, env_var);
    }
}

/*
 * Sets a time string to the current date time using the time string format.
 */
void set_time_string(char *time_string, int size, const char * time_format)
{
    time_t date_timer;
    struct tm *tm_info;
    time(&date_timer);
    tm_info = localtime(&date_timer);
    strftime(time_string, size, time_format, tm_info);
}

/*
 * Sets a time string using the time string format using the time provided.
 */
void set_time_string_with_time(char *time_string, int size, const char * time_format, time_t time_to_format)
{
    struct tm *tm_info;
    tm_info = localtime(&time_to_format);
    strftime(time_string, size, time_format, tm_info);
}

/*
 * Prints a name and value pair aligned to a column width followed by a newline.
 */
void print_name_value(const char *name, const char *value, int column_width)
{
    info("%-*s %s", column_width, name, value);
}

/*
 * Prints the value of an environment variable, or {not set}.
 */
void print_env_var(const char *env_var_name, int col_width)
{
    const char * env_var = getenv(env_var_name);
    const char * value = env_var == NULL ? "{not set}" : env_var;
    print_name_value(env_var_name, value, col_width);
}

/*
 * Prints a list of paths with a header to describe them.
 */
void print_list_of_paths(char *header, char names[][MAX_PATH_LENGTH], int num_names, int col_width)
{
    if (num_names > 0)
    {
        print_name_value(header, names[0], col_width);
        for (int i = 1; i < num_names; i++)
        {
            print_name_value("", names[i], col_width);
        }
    }
    else
    {
        print_name_value(header, "{none defined}", col_width);
    }
}

/*
 * Prints the path to a file with an explanatory header, and warns if access can't be obtained to it.
 */
void print_file_and_access(const char *header, const char *path, const char *filename, int col_width)
{
    char fullpath[MAX_PATH_LENGTH];
    set_full_path(fullpath, path, filename);
    print_name_value(header, fullpath, col_width);

    if (access(fullpath, F_OK) != 0)
    {
        warn("Could not find a %s file at %s", filename, fullpath);
    }
}

/*
 * Prints a help line with consistent formatting.
 */
void print_help_line(const char *description, const char *short_option, const char *long_option, const char *params)
{
    info("%-8s %-18s %-8s %s", short_option, long_option, params, description);
}

/*
 * Loads an individual file given in filename into a buffer, up to a max size of n.
 * Returns the number of characters read from the file.
 */
int load_text_buffer(const char *filename, unsigned char *buffer, int n)
{
    FILE *input = fopen(filename, "r");
    if (input == NULL)
        return -1;

    int i = 0, c;
    while (i < n && (c = getc(input)) != EOF)
        buffer[i++] = c;

    fclose(input);
    return i;
}

/*
 * Fills the field_value of max buffer size with the contents of a tab-delimited field in a line, trimming whitespace off the end.
 * The first field is zero, second field 1, etc.
 * Returns true if it found a value that would fit into the buffer, false otherwise.
 */
int get_tab_field(const char *from_line, int field_no, char *field_value, int buffer_size)
{
    // Find the start of the field:
    const char *field_start = from_line;
    for (int field = 0; field < field_no; field++)
    {
        field_start = strchr(field_start, '\t'); // find the next delimiter.
        if (field_start == NULL) return 0;             // if we can't find a field to start, we didn't succeed.
        field_start++;                                 // actual field starts one beyond the delimiter.
    }

    // Find the end of the field:
    const char *field_end = strchr(field_start, '\t'); // find next delimiter.
    if (field_end == NULL) field_end = strchr(field_start, '\n'); // if no next delimiter, find newline.
    if (field_end == NULL)
        field_end = from_line + strlen(from_line) - 1; // no newline - just use the end of the string.
    else
        field_end--; // field end is one before the newline.

    // Ensure we have something to return:
    if (field_start > field_end) return 0; // no token here.
    size_t len = field_end - field_start + 1;
    if (len + 1 > buffer_size) return 0; // can't fit the token into the buffer (need one extra space for null terminator).

    // Copy the string into the field value and terminate with a null char, then trim whitespace off the end.
    memcpy(field_value, field_start, len);
    field_value[len] = STR_END_CHAR;
    trim_str(field_value);

    return 1;
}

/*
 * Hashes a string using the FNV hash algorithm.
 *
 * See http://www.isthe.com/chongo/tech/comp/fnv/#FNV-1a
 * See https://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash
 *
 * While there are stronger hash algorithms, the virtue of FNV is it is extremely easy to calculate.
 * Its properties rely on the special prime numbers it starts with and multiplies by, modulo 64 bits.
 * We are not looking for cryptographic strength here, just a 64-bit deterministic digest of a sequence of bytes.
 */
unsigned long long hash_string(const char *string)
{
    unsigned long long hash_result = FNV_HASH_OFFSET_64;

    int i = 0;
    char c;
    while ((c=string[i++]))
    {
        hash_result = FNV_HASH_64(c, hash_result);
    }

    return hash_result;
}

/*
 * Hashes the contents of the file at file_path using the FNV hash algorithm, with an optional key (can be empty string).
 *
 * See http://www.isthe.com/chongo/tech/comp/fnv/#FNV-1a
 * See https://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash
 *
 * While there are stronger hash algorithms, the virtue of FNV is it is extremely easy to calculate.
 * Its properties rely on the special prime numbers it starts with and multiplies by, modulo 64 bits.
 * We are not looking for cryptographic strength here, just a 64-bit deterministic digest of a sequence of bytes.
 */
unsigned long long hash_keyed_file(const char *key, const char file_path[MAX_PATH_LENGTH])
{
    unsigned long long hash_result = hash_string(key);

    int i;
    FILE *algo_fp = fopen(file_path, "r");
    if (algo_fp != NULL)
    {
        while ((i = getc(algo_fp) != EOF))
        {
            hash_result = FNV_HASH_64(i, hash_result);
        }
        fclose(algo_fp);
    }

    return hash_result;
}

/*
 * Empties the file passed in.
 */
void empty_file(char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp != NULL) {

        if (ftruncate(fileno(fp), 0))
        {
            warn("Error while truncating the file: %s", filename);
        }
        else
        {
            info("Cleared file: %s", filename);
        }

        fclose(fp);
    }
}

/*
 * Loads the names of algo_names to run from a text file (e.g. selected_algos).
 * Valid names can be no longer than ALGO_NAME_LEN.
 * Returns the number of lines read.
 */
int read_valid_algo_names_from_filename(char lines[][ALGO_NAME_LEN], const char *filename, int max_lines)
{
    int num_lines = 0;

    if (access(filename, F_OK) == 0)
    {
        FILE *algo_file = fopen(filename, "r");

        if (algo_file != NULL)
        {
            char *line = NULL;
            size_t len;

            while (num_lines < max_lines && (getline(&line, &len, algo_file)) != -1)
            {
                trim_str(line);
                size_t length = strlen(line);
                if (length >= ALGO_NAME_LEN)
                {
                    // ALGO_NAME_LEN- 1 because we have to leave room for string terminator zero char.
                    warn("Ignoring algorithm '%s' as the length exceeds the maximum name length: %d", line, ALGO_NAME_LEN - 1);
                }
                else if (length > 0)
                {
                    strcpy(lines[num_lines++], line);
                }
            }

            if (line != NULL)
                free(line);

            fclose(algo_file);
        }
        else
        {
            warn("Could not open file %s for reading.", filename);
        }
    }
    else
    {
        warn("File %s cannot be found.", filename);
    }

    return num_lines;
}

/*
 * Returns the file mode of the file or directory passed in.
 */
__mode_t get_file_mode(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return 0;
    return st.st_mode;
}

/*
 * Searches for a filename in the search paths provided.  The filename can also be a directory name.
 * Sets the full path if found in valid_path, or an empty string otherwise.
 * Returns true if a valid path was found.
 */
int locate_file_path(char valid_path[MAX_PATH_LENGTH], const char *filename, const char search_paths[][MAX_PATH_LENGTH], int num_search_paths)
{
    valid_path[0] = STR_END_CHAR;
    char search_path[MAX_PATH_LENGTH];

    for (int path = 0; path < num_search_paths; path++)
    {
        if (set_full_path(search_path, search_paths[path], filename))
        {
            if (access(search_path, F_OK) == 0)
            {
                strcpy(valid_path, search_path);
                break;
            }
        }
        else
        {
            warn("Path with filename too long - ignoring: %s/%s.", search_paths[path], filename);
        }
    }

    // strip any trailing / from the path.
    size_t path_len = strlen(valid_path);
    if (path_len > 0 && valid_path[path_len - 1] == '/')
    {
        valid_path[path_len - 1] = STR_END_CHAR;
    }

    return !is_empty_string(valid_path);
}

/*
 * Adds all the files that exist in the path into filenames, with the full path provided, up to a maximum number of files.
 * Starts adding files at the filename_index provided.
 * Returns the number of files added.
 */
int add_filenames_in_dir(const char *path, char filenames[][MAX_PATH_LENGTH], int filename_index, int max_files){
    DIR *dir = opendir(path);
    if (dir != NULL) {

        char base_path[MAX_PATH_LENGTH];
        if (copy_path(base_path, path))
        {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL && filename_index < max_files)
            {
                if (entry->d_type == DT_REG) // is it a normal file?
                {
                    if (set_full_path(filenames[filename_index], base_path, entry->d_name))
                    {
                        filename_index++;
                    }
                    else
                    {
                        warn("Ignoring file %s/%s.  Length exceeds maximum file path length %d",
                             entry->d_name, base_path, MAX_PATH_LENGTH);
                    }
                }
            }
        }
        else
        {
            warn("Ignoring file path %s.  Length exceeds maximum file path length %d", path, MAX_PATH_LENGTH);
        }

        closedir(dir);
    }

    return filename_index;
}

/*
 * Returns true if there is no regex to match, or if the regex matches the text.
 */
int matches(const char *text_to_match, regex_t *regex)
{
    return regex == NULL || regexec(regex, text_to_match, 0, NULL, 0) == 0;
}

/*
 * Compiles algorithm name regular expressions into an array of regex_t compiled expressions.
 * It adds an anchor to the start and end of each regex, so the whole regex must match the entire algorithm name.
 * If a prefix is specified, it prepends the prefix to each regular expression.
 * Exits the program with an error if a regular expression cannot be compiled.
 */
//void compile_algo_name_regexes(regex_t *expressions[], const char *prefix, const char * const algo_names[], int n_algos)
void compile_algo_name_regexes(regex_t *expressions[], const char *prefix, const char algo_names[MAX_SELECT_ALGOS][ALGO_REGEX_LEN], int n_algos)
{
    size_t prefixlen = prefix == NULL ? 0 : strlen(prefix);
    for (int i = 0; i < n_algos; i++)
    {
        size_t length = strlen(algo_names[i]);

        // Prefix the expression with ^ and terminate with $ to ensure the regex matches the entire name.
        char anchored_expression[length + 3 + prefixlen];
        anchored_expression[0] = '^'; // anchor to start of string.
        size_t nextpos = 1;
        if (prefixlen > 0) {
            memcpy(anchored_expression + nextpos, prefix, prefixlen);
            nextpos += prefixlen;
        }
        memcpy(anchored_expression + nextpos, algo_names[i], length);
        anchored_expression[length + nextpos] = '$'; //anchor to end of string.
        anchored_expression[length + nextpos + 1] = STR_END_CHAR; // terminate string.

        expressions[i] = (regex_t *) malloc(sizeof(regex_t));
        if (regcomp(expressions[i], anchored_expression, REG_ICASE | REG_EXTENDED) != 0)
        {
            error_and_exit("Could not compile regular expression %s\n\t\tCheck the POSIX extended regex syntax.", algo_names[i]);
        }
    }
}

/*
 * Frees memory in an array of regexes.
 */
void free_regexes(regex_t *expressions[], int num_expressions)
{
    for (int i = 0; i < num_expressions; i++)
    {
        regfree(expressions[i]);
        free(expressions[i]);
        expressions[i] = NULL;
    }
}

/*
 * Returns true if any of the regular expressions match the string provided.
 */
int regexes_match(regex_t *expressions[], int n_expressions, const char *string)
{
    for (int i = 0; i < n_expressions; i++)
    {
        if (regexec(expressions[i], string, 0, NULL, 0) == 0)
        {
            return 1;
        }
    }

    return 0;
}

/*
 * Returns a buffer size big enough to hold the text and additional patterns an algorithm may write at the end,
 * with a bit of extra padding to guard against bugs.
 */
int get_text_buffer_size(int text_size, int max_pat_len)
{
    return sizeof(unsigned char) * (text_size + (NUM_PATTERNS_AT_END_OF_TEXT * max_pat_len) + TEXT_SIZE_PADDING);
}

/*
 * Lists all the filenames in the path with the given suffix, and places them in filenames, stripping out the suffix,
 * starting from the current index.
 */
int add_and_trim_filenames_with_suffix(char filenames[][ALGO_NAME_LEN], const char *path, int current_index, const char *suffix)
{
    int num_file_names = 0;
    DIR *dir = opendir(path);

    if (dir != NULL)
    {
        size_t suffix_len = strlen(suffix);
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_REG && has_suffix(entry->d_name, suffix))
            {
                size_t filename_len = strlen(entry->d_name);
                if (filename_len > suffix_len && filename_len - suffix_len < ALGO_NAME_LEN) {
                    strncpy(filenames[current_index + num_file_names], entry->d_name, filename_len - suffix_len);
                    num_file_names++;
                }
                else if (filename_len - suffix_len >= ALGO_NAME_LEN)
                {
                    // ALGO_NAME_LEN - 1 because we have to leave room for string terminator zero char.
                    warn("Ignoring \"%s\" - filename exceeds maximum algorithm name length %d.", entry->d_name, ALGO_NAME_LEN - 1);
                }
            }
        }

        closedir(dir);
    }

    return num_file_names;
}

/*
 * Finds files with a given suffix in a list of paths, and adds them to the filenames
 */
int add_and_trim_filenames_with_suffix_in_paths(char filenames[][ALGO_NAME_LEN], const char *suffix,
                                                const int num_search_paths, const char search_paths[][MAX_PATH_LENGTH])
{
    int num_files = 0;
    for (int i = 0; i < num_search_paths; i++)
    {
        num_files += add_and_trim_filenames_with_suffix(filenames, search_paths[i], num_files, suffix);
    }

    return num_files;
}

#endif
