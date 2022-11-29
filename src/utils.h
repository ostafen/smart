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

#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <regex.h>

#include "defines.h"

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
    printf("	by Simone Faro, Stefano Scafiti and Thierry Lecroq\n");
    printf("	Last Update: May 2017\n");
    printf("\n");
    printf("	If you use this tool in your research please cite the following paper:\n");
    printf("	| Simone Faro and Thierry Lecroq\n");
    printf("	| The Exact Online String Matching Problem: a Review of the Most Recent Results\n");
    printf("	| ACM Computing Surveys, Vol. 45(2): p.13 (2013)\n");
    printf(" ");
}

/*
 * Prints a formatted message and then exits with return code 1.
 */
void print_format_error_message_and_exit(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    exit(1);
}

/*
 * Prints a standard formatted warning message.
 */
void warn(const char * format, ...)
{
    printf("\tWARN:\t");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

/*
 * Sets the random seed for this run, and prints a message showing the seed.
 */
void set_random_seed(const long random_seed)
{
    srand(random_seed);
    printf("\n\tSetting random seed to %ld.  Use -seed %ld if you need to rerun identically.\n",
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
	long pos = strlen(s) - 1;
	while (pos >= 0)
	{
		char c = s[pos];
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t')
			pos--;
		else
			break;
	}
    s[pos + 1] = '\0';
}

/*
 * Returns the last index of a character from pos in a string, or -1 if not found.
 */
long last_index_of_from_pos(const char *string, char to_find, long pos)
{
    long limit = strlen(string) - 1;
    if (pos > limit) pos = limit;
    while (pos >= 0 && string[pos] != to_find) pos--;
    return pos;
}

/* returns 1 if s is an integer number. 0 otherwise */
int is_int(const char *s)
{
	int i;
	for (i = 0; i < strlen(s); i++)
		if (s[i] < '0' || s[i] > '9')
			return 0;
	return 1;
}

/*
 * Converts string s to lower case.
 */
char *str2lower(char *s)
{
	long n = strlen(s) - 1;
	while (n >= 0)
	{
		if (s[n] >= 'A' && s[n] <= 'Z')
			s[n] = s[n] - 'A' + 'a';
		n--;
	}
	return s;
}

/*
 * Converts string s to upper case.
 */
char *str2upper(char *s)
{
	long n = strlen(s) - 1;
	while (n >= 0)
	{
		if (s[n] >= 'a' && s[n] <= 'z')
			s[n] = s[n] - 'a' + 'A';
		n--;
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
 * Builds the full path given a path (with no trailing /) and a filename.
 * If the length of both exceeds the max path length, an error message is printed and the program exits.
 */
void set_full_path(char fullname[MAX_PATH_LENGTH], const char *path, const char *filename)
{
    if (snprintf(fullname, MAX_PATH_LENGTH, "%s/%s", path, filename) > MAX_PATH_LENGTH)
    {
        print_format_error_message_and_exit("Full path exceeds max path length of %d\n%s/%s",
                                            MAX_PATH_LENGTH, path, filename);
    }
}

/*
 * Sets a time string using the time string format.
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
 * Prints a message followed by the time.
 */
void print_time_message(const char *message)
{
    char time_format[26];
    set_time_string(time_format, 26, "%Y:%m:%d %H:%M:%S");
    printf("%s %s\n", message, time_format);
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
 * Empties the file passed in.
 */
void empty_file(char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp != NULL) {

        if (ftruncate(fileno(fp), 0))
        {
            printf("error while truncating the file: %s\n", filename);
        }
        else
        {
            printf("Cleared file: %s\n", filename);
        }

        fclose(fp);
    }
}

/*
 * Loads the names of algo_names to run from a text file (e.g. selected_algos).
 * Valid names can be no longer than ALGO_NAME_LEN.
 */
int read_valid_algo_names_from_filename(char lines[][ALGO_NAME_LEN], const char *filename, int max_lines)
{
    FILE *algo_file = fopen(filename, "r");

    char *line = NULL;
    size_t len;

    int num_lines = 0;
    while (num_lines < max_lines && (getline(&line, &len, algo_file)) != -1)
    {
        trim_str(line);
        unsigned long length = strlen(line);
        if (length > ALGO_NAME_LEN)
        {
            warn("Ignoring algorithm '%s' as the length exceeds the maximum name length: %d", line, ALGO_NAME_LEN);
        }
        else if (length > 0)
        {
            strcpy(lines[num_lines++], line);
        }
    }

    if (line != NULL)
        free(line);

    fclose(algo_file);

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
 * Returns true (1) if a valid path was found, and 0 (false) if not.
 */
int locate_file_path(char valid_path[MAX_PATH_LENGTH], const char *filename, const char search_paths[][MAX_PATH_LENGTH], int num_search_paths)
{
    valid_path[0] = '\0';
    char search_path[MAX_PATH_LENGTH];

    for (int path = 0; path < num_search_paths; path++)
    {
        unsigned long len = strlen(search_paths[path]);
        if (len + strlen(filename) < MAX_PATH_LENGTH - 1) {

            // check to see if search path already terminates with /
            if (search_paths[path][len - 1] == '/')
            {
                snprintf(search_path, MAX_PATH_LENGTH,"%s%s", search_paths[path], filename);
            } else {
                snprintf(search_path, MAX_PATH_LENGTH, "%s/%s", search_paths[path], filename);
            }

            if (access(search_path, F_OK) == 0)
            {
                strcpy(valid_path, search_path);
                break;
            }
        }
        else
        {
            warn("Length of search path %s and filename %s exceeds maximum file path length - ignoring search path.",
                   search_paths[path], filename);
        }
    }

    // ensure that valid path does not end with a /
    unsigned long path_len = strlen(valid_path);
    if (path_len > 0 && valid_path[path_len - 1] == '/')
    {
        valid_path[path_len - 1] = '\0';
    }

    return valid_path[0] != '\0';
}

/*
 * Adds all the files that exist in the path into filenames, with the full path provided, up to a maximum number of files.
 * Starts adding files at the filename_index provided.
 * Returns the number of files added.
 */
int add_filenames_in_dir(const char *path, char filenames[][MAX_PATH_LENGTH], int filename_index, int max_files){
    DIR *dir = opendir(path);
    if (dir != NULL) {

        char full_path[MAX_PATH_LENGTH];
        strcpy(full_path, path);
        unsigned long path_len = strlen(path);
        full_path[path_len++] = '/';
        full_path[path_len] = '\0';

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && filename_index < max_files)
        {
            if (entry->d_type == DT_REG)
            {
                unsigned long entry_len = strlen(entry->d_name);
                if (path_len + entry_len < MAX_PATH_LENGTH)
                {
                    strcpy(full_path + path_len, entry->d_name);
                    strcpy(filenames[filename_index++], full_path);
                }
                else
                {
                    warn("File %s and path %s exceed maximum path length %d - cannot process.",
                           entry->d_name, full_path, MAX_PATH_LENGTH);
                }
            }
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
 */
int compile_algo_name_regexes(regex_t *expressions[], const char * const algo_names[], int n_algos)
{
    for (int i = 0; i < n_algos; i++)
    {
        unsigned long length = strlen(algo_names[i]);
        char anchored_expression[length + 3];
        anchored_expression[0] = '^'; // anchor to start of string.
        memcpy(anchored_expression + 1, algo_names[i], length);
        anchored_expression[length + 1] = '$'; //anchor to end of string.
        anchored_expression[length + 2] = '\0';

        if (regcomp(expressions[i], anchored_expression, REG_ICASE | REG_EXTENDED) != 0)
        {
            print_format_error_message_and_exit("Could not compile regular expression %s", algo_names[i]);
        }
    }

    return 0;
}

/*
 * Frees memory in an array of regexes.
 */
void free_regexes(regex_t *expressions[], int num_expressions)
{
    for (int i = 0; i < num_expressions; i++)
        regfree(expressions[i]);
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
 * Lists all the filenames in the path with the given suffix, and places them in filenames, stripping out the suffix,
 * starting from the current index.
 */
int add_and_trim_filenames_with_suffix(char filenames[][ALGO_NAME_LEN], const char *path, int current_index, const char *suffix)
{
    int num_file_names = 0;
    DIR *dir = opendir(path);

    if (dir != NULL)
    {
        const unsigned long suffix_len = strlen(suffix);
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_REG && has_suffix(entry->d_name, suffix))
            {
                unsigned long filename_len = strlen(entry->d_name);
                if (filename_len - suffix_len > 0) {
                    strncpy(filenames[current_index + num_file_names], entry->d_name, filename_len - suffix_len);
                    num_file_names++;
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