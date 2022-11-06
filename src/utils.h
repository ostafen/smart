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

static int str_compare(const void *str1, const void *str2)
{
	if (strcmp(str1, str2) >= 0)
		return 1;
	else
		return 0;
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

#define STR_BUF 100

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

#define STR_BUF 100
#define MAX_FILE_LINES 1000

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