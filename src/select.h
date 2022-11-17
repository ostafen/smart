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
#include "regex.h"

#define SMART_BIN_FOLDER "./bin/algos"

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

void write_file(const char *lines[MAX_SELECT_ALGOS], int num_lines)
{
    FILE *tmp_fp = fopen("selected_algos.tmp", "ab+");

    for (int i = 0; i < num_lines; i++)
    {
        fprintf(tmp_fp, "%s\n", lines[i]);
    }

    rename("selected_algos.tmp", "selected_algos");
    remove("selected_algos.tmp");

    fclose(tmp_fp);
}

int compile_regexes(regex_t *expressions, const char **algos, int n_algos)
{
    for (int i = 0; i < n_algos; i++)
    {
        int length = strlen(algos[i]);
        char anchored_expression[length + 3];
        anchored_expression[0] = '^'; // anchor to start of string.
        memcpy(anchored_expression + 1, algos[i], length);
        anchored_expression[length + 1] = '$'; //anchor to end of string.
        anchored_expression[length + 2] = '\0';

        if (regcomp(&(expressions[i]), anchored_expression, REG_ICASE | REG_EXTENDED) != 0)
        {
            print_format_error_message_and_exit("Could not compile regular expression %s", algos[i]);
        }
    }

    return 0;
}

int regexes_match(regex_t *expressions, int n_expressions, const char *string)
{
    for (int i = 0; i < n_expressions; i++)
    {
        if (regexec(&(expressions[i]), string, 0, NULL, 0) == 0)
        {
            return 1;
        }
    }

    return 0;
}

void add_algos(FILE *fp, const char **algos, int n_algos)
{
    // Compile regular expressions:
    regex_t regexes[n_algos];
    compile_regexes(regexes, algos, n_algos);

    // Build set of currently selected algorithms:
    char selected_algos[MAX_SELECT_ALGOS][STR_BUF];
    int n_selected_algos = read_all_lines(fp, selected_algos);
    str_set_t set;
    str_set_init(&set);

    for (int i = 0; i < n_selected_algos; i++) {
        str2lower(selected_algos[i]);
        str_set_add(&set, selected_algos[i]);
    }

    // Build list of compiled algo shared objects
    char filenames[MAX_FILE_LINES][MAX_PATH_LENGTH];
    int n_compiled_algos = list_dir(SMART_BIN_FOLDER, filenames, DT_REG, 0);

    // Check each compiled algorithm name against our regexes.  If they match, and we don't already have it, add it.
    int matched = 0;
    for (int i = 0; i < n_compiled_algos; i++)
    {
        trim_suffix(filenames[i], ".so");
        if (regexes_match(regexes, n_algos, filenames[i]))
        {
            if (str_set_contains(&set, filenames[i]))
            {
                printf("Algorithm %s already in the set.\n", filenames[i]);
            }
            else {
                printf("Adding algorithm %s to set.\n", filenames[i]);
                fprintf(fp, "%s\n", filenames[i]);
                matched = 1;
            }
        }
    }

    if (!matched)
        printf("Did not find any matching algorithms not already in the selected set.\n");

    // Free memory.
    str_set_free(&set);
    for (int i = 0; i < n_algos; i++)
        regfree(&(regexes[i]));
}

void remove_algos(FILE *fp, const char **algos, int n_algos)
{
	char selected_algos[MAX_SELECT_ALGOS][STR_BUF];
	int n_selected_algos = read_all_lines(fp, selected_algos);

    // Compile regular expressions:
    regex_t regexes[n_algos];
    compile_regexes(regexes, algos, n_algos);

    // Find lines that don't match any of the regexes.
    const char * remaining_selected_algos[MAX_SELECT_ALGOS];
    int remain_count = 0;
    for (int i = 0; i < n_selected_algos; i++)
    {
        if (!regexes_match(regexes, n_algos, selected_algos[i]))
        {
            remaining_selected_algos[remain_count++] = selected_algos[i];
        } else {
            printf("Removing %s from selected algorithms.\n", selected_algos[i]);
        }
    }

    // If we have fewer lines that we started with, rewrite the file.
    if (remain_count < n_selected_algos)
    {
        write_file(remaining_selected_algos, remain_count);
    }
    else
    {
        printf("Did not find any algorithms to remove in the selected set.\n");
    }

    // Free regex memory.
    for (int i = 0; i < n_algos; i++)
        regfree(&(regexes[i]));
}

void list_algo_file(FILE *fp)
{
	char algos[MAX_SELECT_ALGOS][STR_BUF];

	int n = read_all_lines(fp, algos);
	for (int i = 0; i < n; i++)
		printf("%s\n", algos[i]);
}

void list_selectable_algos()
{
	char filenames[MAX_FILE_LINES][MAX_PATH_LENGTH];
	int n = list_dir(SMART_BIN_FOLDER, filenames, DT_REG, 0);
	if (n < 0)
	{
		printf("unable to list dir\n");
		exit(1);
	}

	qsort(filenames, n, sizeof(char) * MAX_PATH_LENGTH, str_compare);

	for (int i = 0; i < n; i++)
	{
		trim_suffix(filenames[i], ".so");
		str2upper(filenames[i]);
		printf("%s\n", filenames[i]);
	}
}

int exec_select(select_command_opts_t *opts, smart_config_t *smart_config)
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
		} else
        {
            printf("Cleared selected algorithms.\n");
        }
	}

	if (fp)
		fclose(fp);

    return 0;
}
