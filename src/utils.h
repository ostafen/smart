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

static int str_compare(const void *str1, const void *str2)
{
	if (strcmp(str1, str2) >= 0)
		return 1;
	else
		return 0;
}

int strcmpany(const char *s, int n, ...)
{
    va_list ptr;

    va_start(ptr, n);
    for (int i = 0; i < n; i++)
    {
        const char *curr_str = va_arg(ptr, const char *);
        if (!strcmp(curr_str, s))
            return 0;
    }
    va_end(ptr);

    return -1;
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
	int pos = strlen(s) - 1;
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
	int n = strlen(s) - 1;
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
	int n = strlen(s) - 1;
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
	int m = strlen(filename);
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

#define STR_BUF 512

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

#define MAX_FILE_LINES 1000

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

void locate_file_path(char valid_path[STR_BUF], const char *filename, const char search_paths[][STR_BUF], int num_search_paths)
{
    // Check to see if file or dir exists without using search path:
    char working_dir[STR_BUF];
    char local_file[STR_BUF];
    if (getcwd(working_dir, STR_BUF) == NULL)
    {
        strcpy(local_file, filename);
    } else {
        strcpy(local_file, working_dir);
        int dirlen = strlen(local_file);
        if (dirlen > 0 && local_file[dirlen - 1] != '/')
        {
            strcat(local_file, "/");
        }
        strcat(local_file, filename);
    }

    if (access(local_file, F_OK) == 0)
    {
        strcpy(valid_path, local_file);
    }
    else {  // Check search paths:
        valid_path[0] = '\0';
        char search_path[STR_BUF];

        for (int path = 0; path < num_search_paths; path++) {
            int len = strlen(search_paths[path]);
            if (len + strlen(filename) < STR_BUF - 1) {

                if (search_paths[path][len - 1] == '/') { // check to see if search path already terminates with /
                    snprintf(search_path, STR_BUF,"%s%s", search_paths[path], filename);
                } else {
                    snprintf(search_path, STR_BUF, "%s/%s", search_paths[path], filename);
                }

                if (access(search_path, F_OK) == 0) {
                    strcpy(valid_path, search_path);
                    break;
                }
            } else {
                printf("\t: Ignoring: Length of search path %s and filename %s exceeds maximum file path length:",
                       search_paths[path], filename);
            }
        }
    }

    // ensure that valid path does not end with a /
    int path_len = strlen(valid_path);
    if (path_len > 0 && valid_path[path_len - 1] == '/')
    {
        valid_path[path_len - 1] = '\0';
    }
}

int add_filenames_in_dir(const char *path, char filenames[][STR_BUF], int filename_index, int max_files){
    DIR *dir = opendir(path);
    if (dir != NULL) {

        char full_path[STR_BUF];
        strcpy(full_path, path);
        int path_len = strlen(path);
        full_path[path_len++] = '/';
        full_path[path_len] = '\0';

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && filename_index < max_files)
        {
            if (entry->d_type == DT_REG)
            {
                int entry_len = strlen(entry->d_name);
                if (path_len + entry_len < STR_BUF)
                {
                    strcpy(full_path + path_len, entry->d_name);
                    strcpy(filenames[filename_index++], full_path);
                }
                else {
                    printf("\tFile %s and path %s exceed maximum path length %d - cannot process.", entry->d_name, full_path, STR_BUF);
                }
            }
        }

        closedir(dir);
    }

    return filename_index;
}

int split_search_paths(const char *search_path, char search_paths[][STR_BUF], int max_paths)
{
    const char *path_delimitter = ":";
    int pathlen = strlen(search_path);
    char tokenised_search_path[pathlen + 1];
    strcpy(tokenised_search_path, search_path);

    int path_count = 0;
    char *path = strtok(tokenised_search_path, path_delimitter);
    while (path != NULL && path_count < max_paths)
    {
        strcpy(search_paths[path_count++], path);
        path = strtok(NULL, path_delimitter);
    }

    return path_count;
}

int list_dir(const char *path, char filenames[][STR_BUF], int f_type, int include_path)
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
			char full_path[STR_BUF];
			memset(full_path, 0, sizeof(char) * STR_BUF);

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