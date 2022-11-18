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

#define STR_BUF 512
#define MAX_FILE_LINES 2048
#define MAX_PATH_LENGTH 2048

static int str_compare(const void *str1, const void *str2)
{
	if (strcmp(str1, str2) >= 0)
		return 1;
	else
		return 0;
}

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

int has_suffix(const char *s, const char *suffix)
{
	return !strcmp(s + strlen(s) - strlen(suffix), suffix);
}

void trim_suffix(char *s, const char *suffix)
{
	if (has_suffix(s, suffix))
		s[strlen(s) - strlen(suffix)] = '\0';
}

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

void set_time_string(char *time_string, int size, const char * time_format)
{
    time_t date_timer;
    struct tm *tm_info;
    time(&date_timer);
    tm_info = localtime(&date_timer);
    strftime(time_string, size, time_format, tm_info);
}

int split_filename(const char *filename, char list_of_filenames[500][50])
{
	int i, j, k;
	i = j = k = 0;
	size_t m = strlen(filename);
	while (i < m)
	{
		while (filename[i] != '-' && i < m)
		{
			list_of_filenames[k][j++] = filename[i];
			i++;
		}
		list_of_filenames[k][j] = '\0';
		if (filename[i] == '-')
		{
			k++;
			j = 0;
			i++;
		}
	}
	return (k + 1);
}

int read_all_lines(FILE *fp, char output[][STR_BUF])
{
	char *line = NULL;
	size_t len;

	int k = 0;
	while ((getline(&line, &len, fp)) != -1)
	{
		trim_str(line);
        if (strlen(line) > 0)
        {
            strcpy(output[k++], line);
        }
	}
	return k;
}

__mode_t get_file_mode(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return 0;
    return st.st_mode;
}

/*
 * Returns the size of the file given in path.
 */
size_t fsize(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return -1;
    return st.st_size;
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
        long len = strlen(search_paths[path]);
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
            printf("\tWARN\tLength of search path %s and filename %s exceeds maximum file path length - ignoring search path.\n",
                   search_paths[path], filename);
        }
    }

    // ensure that valid path does not end with a /
    long path_len = strlen(valid_path);
    if (path_len > 0 && valid_path[path_len - 1] == '/')
    {
        valid_path[path_len - 1] = '\0';
    }

    return valid_path[0] != '\0';
}

int add_filenames_in_dir(const char *path, char filenames[][MAX_PATH_LENGTH], int filename_index, int max_files){
    DIR *dir = opendir(path);
    if (dir != NULL) {

        char full_path[MAX_PATH_LENGTH];
        strcpy(full_path, path);
        long path_len = strlen(path);
        full_path[path_len++] = '/';
        full_path[path_len] = '\0';

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && filename_index < max_files)
        {
            if (entry->d_type == DT_REG)
            {
                long entry_len = strlen(entry->d_name);
                if (path_len + entry_len < MAX_PATH_LENGTH)
                {
                    strcpy(full_path + path_len, entry->d_name);
                    strcpy(filenames[filename_index++], full_path);
                }
                else
                {
                    printf("\tWARN\tFile %s and path %s exceed maximum path length %d - cannot process.",
                           entry->d_name, full_path, MAX_PATH_LENGTH);
                }
            }
        }

        closedir(dir);
    }

    return filename_index;
}

int list_dir(const char *path, char filenames[][MAX_PATH_LENGTH], int f_type, int include_path)
{
	DIR *dir = opendir(path);
	if (dir == NULL)
		return -1;

	int n = 0;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == f_type)
		{
			// TODO: extract function join_path();
			char full_path[MAX_PATH_LENGTH];
			memset(full_path, 0, sizeof(char) * MAX_PATH_LENGTH);

			if (include_path)
			{
				strcat(full_path, path);
				strcat(full_path, "/");
			}
			strcat(full_path, entry->d_name);

			strcpy(filenames[n++], full_path);
		}
	}

	if (closedir(dir) < 0)
		return -1;

	return n;
}

#endif