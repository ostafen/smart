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

#define SELECTED_ALGOS_FILENAME "selected_algos"

static const char *ALGO_COLUMN_FORMAT = "%-18s ";
static const int ALGO_NUM_COLUMNS = 6;

/*
 * Writes the strings contained in lines up to num_lines to the current selected algorithms file.
 */
void write_lines_to_selected_algos(const char **lines, int num_lines, const smart_config_t *smart_config)
{
    char selected_file_name[MAX_PATH_LENGTH];
    snprintf(selected_file_name, MAX_PATH_LENGTH, "%s/%s", smart_config->smart_config_dir, SELECTED_ALGOS_FILENAME);
    write_lines_to_file(lines, num_lines, selected_file_name);
}

/*
 * Compiles algorithm name regular expressions into an array of regex_t compiled expressions.
 * It adds an anchor to the start and end of each regex, so the whole regex must match the entire algorithm name.
 */
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

/*
 * Returns true if any of the regular expressions match the string provided.
 */
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

/*
 * Removes algorithms from the selected set, by matching to POSIX extended regular expressions.
 * Any algorithm in the selected set that matches any of the regular expressions will be removed.
 */
void remove_algos(const char **algos, int n_algos, const smart_config_t *smart_config)
{
    char selected_algos[MAX_SELECT_ALGOS][STR_BUF];
    char selected_algos_path[MAX_PATH_LENGTH];
    snprintf(selected_algos_path, MAX_PATH_LENGTH, "%s/%s", smart_config->smart_config_dir, SELECTED_ALGOS_FILENAME);

    FILE *fp = fopen(selected_algos_path, "r");
    if (fp != NULL)
    {
        int n_selected_algos = read_all_lines(fp, selected_algos, MAX_SELECT_ALGOS);
        fclose(fp);

        // Compile regular expressions:
        regex_t regexes[n_algos];
        compile_regexes(regexes, algos, n_algos);

        // Find lines that don't match any of the regexes.
        const char *remaining_selected_algos[MAX_SELECT_ALGOS];
        int remain_count = 0;
        int matched = 0;
        for (int i = 0; i < n_selected_algos; i++)
        {
            if (!regexes_match(regexes, n_algos, selected_algos[i]))
            {
                remaining_selected_algos[remain_count++] = selected_algos[i];
            } else
            {
                if (!matched)
                    printf("Removing %s", selected_algos[i]);
                else
                    printf(", %s", selected_algos[i]);
                matched = 1;
            }
        }

        if (matched)
            printf(".\n");

        // If we have fewer lines that we started with, rewrite the file.
        if (remain_count < n_selected_algos)
        {
            write_lines_to_selected_algos(remaining_selected_algos, remain_count, smart_config);
        } else
        {
            printf("Did not find any algorithms to remove in the selected set.\n");
        }

        // Free regex memory.
        for (int i = 0; i < n_algos; i++)
            regfree(&(regexes[i]));
    }
    else
    {
        printf("\tWARN\tCould not open selected algos file: %s\n", selected_algos_path);
    }
}

/*
 * Returns a pointer to the selected algorithm file opened with the file_mode, or NULL if it could not be opened.
 */
FILE *open_selected_algorithm_file(const smart_config_t *smart_config, const char *file_mode)
{
    char selectable_algos_filename[MAX_PATH_LENGTH];
    snprintf(selectable_algos_filename, MAX_PATH_LENGTH, "%s/%s", smart_config->smart_config_dir, SELECTED_ALGOS_FILENAME);

    return fopen(selectable_algos_filename, file_mode);
}

int read_all_lines_in_selected_algo_file(const smart_config_t *smart_config, char lines[][STR_BUF], int max_lines)
{
    FILE *fp = open_selected_algorithm_file(smart_config, "r");
    int num_read = 0;
    if (fp != NULL) {
        num_read = read_all_lines(fp, lines, max_lines);
        fclose(fp);
    }
    return num_read;
}

/*
 * Adds all shared object files found in the path to filenames, starting from the current_index of filenames.
 * Returns the number of filenames added.
 */
int add_shared_objects(const char *path, char filenames[][MAX_PATH_LENGTH], int current_index)
{
    regex_t filename_filter;
    regcomp(&filename_filter, ".so$", REG_EXTENDED);
    int num_added = add_filenames_in_path(path, filenames, current_index, &filename_filter);
    regfree(&filename_filter);

    return num_added;
}

/*
 * Finds selectable algorithm shared objects in all the algo search paths defined in the config.
 * Returns the number of selectable algorithms found.
 */
int find_selectable_algos(char filenames[][MAX_PATH_LENGTH], const smart_config_t *smart_config)
{
    int num_selectable_algos = 0;
    for (int i = 0; i < smart_config->num_algo_search_paths; i++)
    {
        num_selectable_algos += add_shared_objects(smart_config->smart_algo_search_paths[i], filenames, num_selectable_algos);
    }

    return num_selectable_algos;
}

int add_algos_matching_regexes_to_set(str_set_t *selected_algo_set, char filenames[][MAX_PATH_LENGTH],
                                       int num_selectable_algos, regex_t regexes[], int num_regexes, const smart_config_t *smart_config)
{
    int matched = 0;
    for (int i = 0; i < num_selectable_algos; i++)
    {
        trim_suffix(filenames[i], ".so");

        if (regexes_match(regexes, num_regexes, filenames[i]))
        {
            if (!str_set_contains(selected_algo_set, filenames[i]))
            {
                if (!matched) {
                    printf("Adding %s", filenames[i]);
                } else
                {
                    printf(", %s", filenames[i]);
                }
                str_set_add(selected_algo_set, filenames[i]);
                matched = 1;
            }
        }
    }

    if (matched)
        printf(".\n");
    else
        printf("Did not find any new algorithms to add to the selected set.\n");

    return matched;
}

void add_algos(const char **algos, int n_algos, const smart_config_t *smart_config)
{
    // Read all the lines in the selected algo file and add them to a set.
    str_set_t selected_algo_set;
    str_set_init(&selected_algo_set);
    char lines[MAX_SELECT_ALGOS][STR_BUF];
    int num_read = read_all_lines_in_selected_algo_file(smart_config, lines, MAX_SELECT_ALGOS);
    for (int i = 0; i < num_read; i++) {
        str2lower(lines[i]); //TODO: always adds as lower case - but we convert to upper case as well?  Review case handling.
        str_set_add(&selected_algo_set, lines[i]);
    }

    // Build list of available compiled algo shared objects on all algo search paths:
    char filenames[MAX_FILE_LINES][MAX_PATH_LENGTH];
    int num_selectable_algos = find_selectable_algos(filenames, smart_config);
    qsort(filenames, num_selectable_algos, sizeof(char) * MAX_PATH_LENGTH, str_compare);

    // Check each compiled algorithm name against our regexes.  If they match, and we don't already have it, add it to the set.
    regex_t regexes[n_algos];
    compile_regexes(regexes, algos, n_algos);
    if (add_algos_matching_regexes_to_set(&selected_algo_set, filenames, num_selectable_algos,
                                           regexes, n_algos, smart_config))
    {
        // We matched some new algos - get the algos in the set, sort them, and write them out to a new selected_algos file.
        const char *new_algos[MAX_SELECT_ALGOS];
        int num_algos = str_set_to_array(&selected_algo_set, new_algos, MAX_SELECT_ALGOS);

        //TODO: sorting not working - probably have some confusion here over pointers or something...
 //       qsort(new_algos, num_algos, sizeof(const char *), str_compare);

        write_lines_to_selected_algos(new_algos, num_algos, smart_config);
    }

    // Free memory.
    str_set_free(&selected_algo_set);
    for (int i = 0; i < n_algos; i++)
        regfree(&(regexes[i]));
}

/*
 * Overwrites the current selected_algos file with the contents of a new file.
 */
void load_selected_algos(const char **algos, int n_algos, const smart_config_t *smart_config)
{
    //TODO:
}

/*
 * Saves the selected_algos file to a new file, with an .algos extension.
 * We impose the .algos extension to prevent accidentally overwriting an important file.
 */
void save_selected_algos(const char *save_name, const smart_config_t *smart_config)
{
    char selected_algo_filename[MAX_PATH_LENGTH];
    if (snprintf(selected_algo_filename, MAX_PATH_LENGTH, "%s/%s", smart_config->smart_config_dir, SELECTED_ALGOS_FILENAME) > MAX_PATH_LENGTH)
        abort(); //TODO: error messages.

    char save_filename[MAX_PATH_LENGTH];
    if (snprintf(save_filename, MAX_PATH_LENGTH, "%s/%s.algos", smart_config->smart_config_dir, save_name) > MAX_PATH_LENGTH)
        abort(); //TODO: error messages.

    copy_text_file(selected_algo_filename, save_filename);
    printf("Saved current selected algorithms as %s\n", save_name);
}

/*
 * Empties the selected_algos file.
 */
void empty_selected_algos(const smart_config_t *smart_config)
{
    FILE *fp = open_selected_algorithm_file(smart_config, "w");
    if (fp != NULL) {

        if (ftruncate(fileno(fp), 0))
        {
            printf("error while truncating the file\n");
        }
        else
        {
            printf("Cleared selected algorithms.\n");
        }

        fclose(fp);
    }
}

/*
 * Prints the algorithms in the selected_algos file to the console, in a tabular format.
 */
void list_selected_algo_file(const smart_config_t *smart_config)
{
    FILE *fp = open_selected_algorithm_file(smart_config, "r");
    if (fp != NULL) {

        char algos[MAX_SELECT_ALGOS][STR_BUF];
        int n = read_all_lines(fp, algos, MAX_SELECT_ALGOS);
        fclose(fp);

        for (int i = 0; i <= n - ALGO_NUM_COLUMNS; i += ALGO_NUM_COLUMNS)
        {
            for (int j = i; j < i + ALGO_NUM_COLUMNS; j++)
            {
                printf(ALGO_COLUMN_FORMAT, algos[j]);
            }
            printf("\n");
        }

        int remaining_columns = n % ALGO_NUM_COLUMNS;
        if (remaining_columns > 0)
        {
            for (int i = n - remaining_columns; i < n; i++)
            {
                printf(ALGO_COLUMN_FORMAT, algos[i]);
            }
            printf("\n");
        }
    }
}

/*
 * Prints all selectable algorithm shared objects in all the algo search paths defined in the config,
 * in a tabular format.
 */
void list_selectable_algos(const smart_config_t *smart_config) {
    char filenames[MAX_FILE_LINES][MAX_PATH_LENGTH];
    int num_algos = find_selectable_algos(filenames, smart_config);

    if (num_algos > 0)
    {
        qsort(filenames, num_algos, sizeof(char) * MAX_PATH_LENGTH, str_compare);

        for (int i = 0; i < num_algos - ALGO_NUM_COLUMNS; i += ALGO_NUM_COLUMNS)
        {
            for (int j = i; j < i + ALGO_NUM_COLUMNS; j++)
            {
                trim_suffix(filenames[j], ".so");
                str2upper(filenames[j]);
                printf(ALGO_COLUMN_FORMAT, filenames[j]);
            }
            printf("\n");
        }

        int remaining_columns = num_algos % ALGO_NUM_COLUMNS;
        if (remaining_columns > 0)
        {
            for (int i = num_algos - remaining_columns; i < num_algos; i++)
            {
                trim_suffix(filenames[i], ".so");
                str2upper(filenames[i]);
                printf(ALGO_COLUMN_FORMAT, filenames[i]);
            }
            printf("\n");
        }
    }
    else
    {
        printf("\tWARN\tUnable to find algos.\n"); //TODO: improve error message.
    }
}

/*
 * Runs the correct select command given the select options.
 */
int exec_select(select_command_opts_t *opts, const smart_config_t *smart_config)
{
	switch (opts->select_command)
    {
        case SHOW_ALL:
        {
            list_selectable_algos(smart_config);
            break;
        }
        case SHOW_SELECTED:
        {
            list_selected_algo_file(smart_config);
            break;
        }
        case DESELECT_ALL:
        {
            empty_selected_algos(smart_config);
            break;
        }
        case LOAD:
        {
            load_selected_algos(opts->algos, opts->n_algos, smart_config);
            break;
        }
        case SAVE:
        {
            save_selected_algos(opts->algos[0], smart_config);
            break;
        }
        case ADD:
        {
            add_algos(opts->algos, opts->n_algos, smart_config);
            break;
        }
        case REMOVE:
        {
            remove_algos(opts->algos, opts->n_algos, smart_config);
            break;
        }
        default:
        {
            print_format_error_message_and_exit("Unknown select command enountered: %d", opts->select_command);
        }
    }

    return 0;
}
