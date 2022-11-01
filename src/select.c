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
#include "string_set.h"

FILE *open_algo_file(const char *filename)
{
	return fopen(filename, "ab+");
}

#define STR_BUF 100

int list_algos_from_file(FILE *fp, const char output[][STR_BUF])
{
	char *line = NULL;
	size_t len;

	int k = 0;
	while ((getline(&line, &len, fp)) != -1)
	{
		trim_str(line);
		strcpy(output[k++], line);
	}
	return k;
}

void add_algos(FILE *fp, const char **algos, int n_algos)
{
	char selected_algos[MAX_SELECT_ALGOS][STR_BUF];
	int n_selected_algos = list_algos_from_file(fp, selected_algos);

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
	int n_selected_algos = list_algos_from_file(fp, selected_algos);

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

	int n = list_algos_from_file(fp, algos);
	for (int i = 0; i < n; i++)
		printf("%s\n", algos[i]);
}

int exec_select(select_command_opts_t *opts)
{
	if (opts->show_all)
	{
		// shows all selected algorithms
		printf("\n\tThe list of selected algorithms:\n");
		printf("\n");
		return 0;
	}

	FILE *fp = open_algo_file("selected_algos");
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
