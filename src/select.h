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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "string_set.h"
#include "parser.h"
#include "utils.h"

FILE *open_algo_file(const char *filename)
{
	return fopen(filename, "ab+");
}

#define MAX_SELECT_ALGOS 100

void add_algos(FILE *fp, const char **algos, int n_algos)
{
	char selected_algos[MAX_SELECT_ALGOS][STR_BUF];
	int n_selected_algos = read_all_lines(fp, selected_algos);

	str_set_t set;
	str_set_init(&set);

	for (int i = 0; i < n_selected_algos; i++)
		str_set_add(&set, selected_algos[i]);

	for (int i = 0; i < n_algos; i++)
	{
		if (!str_set_contains(&set, algos[i]))
		{
			printf("adding %s algo to the set.\n", algos[i]);
			fprintf(fp, "%s\n", algos[i]);
			str_set_add(&set, algos[i]);
		}
		else
		{
			printf("%s algo already in the set.\n", algos[i]);
		}
	}

	str_set_free(&set);
}

void copy_lines(FILE *dst_fp, FILE *src_fp, str_set_t *exclude_set)
{
	char *line = NULL;
	size_t len;
	while ((getline(&line, &len, src_fp)) != -1)
	{
		trim_str(line);

		if (!str_set_contains(exclude_set, line))
			fprintf(dst_fp, "%s\n", line);
	}
}

void rewrite_file_without_strings(FILE *fp, const char **algos, int n)
{
	FILE *tmp_fp = fopen("selected_algos.tmp", "ab+");

	str_set_t algos_set;
	str_set_init(&algos_set);

	for (int i = 0; i < n; i++)
		str_set_add(&algos_set, algos[i]);

	copy_lines(tmp_fp, fp, &algos_set);

	rename("selected_algos.tmp", "selected_algos");
	remove("selected_algos.tmp");

	fclose(tmp_fp);
	str_set_free(&algos_set);
}

void remove_algos(FILE *fp, const char **algos, int n_algos)
{
	char selected_algos[MAX_SELECT_ALGOS][STR_BUF];
	int n_selected_algos = read_all_lines(fp, selected_algos);

	str_set_t selected_algos_set;
	str_set_init(&selected_algos_set);

	for (int i = 0; i < n_selected_algos; i++)
		str_set_add(&selected_algos_set, selected_algos[i]);

	// check that we need to remove at leastone algorithm
	int rewrite_file = 0;
	for (int i = 0; i < n_algos; i++)
	{
		if (str_set_contains(&selected_algos_set, algos[i]))
		{
			rewrite_file = 1;
			break;
		}
	}

	FILE *tmp_fp = fopen("selected_algos.tmp", "ab+");
	if (rewrite_file)
	{
		rewind(fp);
		rewrite_file_without_strings(fp, algos, n_algos);
	}

	str_set_free(&selected_algos_set);
}

void list_algo_file(FILE *fp)
{
	char algos[MAX_SELECT_ALGOS][STR_BUF];

	int n = read_all_lines(fp, algos);
	for (int i = 0; i < n; i++)
		printf("%s\n", algos[i]);
}

#define SMART_BIN_FOLDER "./bin/algos"

void list_selectable_algos()
{
	char filenames[MAX_FILE_LINES][STR_BUF];
	int n = list_dir(SMART_BIN_FOLDER, filenames, DT_REG, 0);
	if (n < 0)
	{
		printf("unable to list dir\n");
		exit(1);
	}

	qsort(filenames, n, sizeof(char) * STR_BUF, str_compare);

	for (int i = 0; i < n; i++)
	{
		trim_suffix(filenames[i], ".so");
		str2upper(filenames[i]);
		printf("%s\n", filenames[i]);
	}
}

int exec_select(select_command_opts_t *opts)
{
	if (opts->show_all)
	{
		list_selectable_algos();
		return 0;
	}

	FILE *fp = fopen("selected_algos", "ab+");
	if (opts->n_algos > 0)
	{
		if (opts->add)
			add_algos(fp, opts->algos, opts->n_algos);
		else
			remove_algos(fp, opts->algos, opts->n_algos);
	}
	else if (opts->show_selected)
	{
		list_algo_file(fp);
	}
	else if (opts->deselect_all)
	{
		if (ftruncate(fileno(fp), 0))
		{
			printf("error while truncating the file\n");
			return 1;
		}
	}

	if (fp)
		fclose(fp);
}
