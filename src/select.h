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
#include "defines.h"
#include "utils.h"
#include "config.h"
#include "parser.h"
#include "algorithms.h"

/*
 * Adds algorithms that match the regex algo names to the selected set.
 */
void add_algos(const char **algos, int n_algos, const smart_config_t *smart_config)
{
    algo_info_t matching_algos;
    get_all_algo_names(smart_config, &matching_algos);
    filter_out_names_not_matching_regexes(&matching_algos, NULL, algos, n_algos);

    int num_merged = 0;
    if (matching_algos.num_algos > 0)
    {
        algo_info_t selected_algos;
        read_algo_names_from_file(smart_config, &selected_algos, SELECTED_ALGOS_FILENAME);

        algo_info_t new_algos;
        num_merged = merge_algorithms(&selected_algos, &matching_algos, &new_algos);

        if (num_merged > 0)
        {
            sort_algorithm_names(&new_algos);
            print_algorithms_as_list("Adding algorithms ", &new_algos);

            sort_algorithm_names(&selected_algos);
            write_algo_names_to_file(smart_config, &selected_algos, SELECTED_ALGOS_FILENAME);
        }
    }

    if (num_merged == 0)
    {
        printf("No new algorithms were found to add to the existing set.\n");
    }
}

/*
 * Removes any algorithms that match the regex algo names from the selected set.
 */
void remove_algos(const char **algos, int n_algos, const smart_config_t *smart_config)
{
    algo_info_t selected_algos;
    read_algo_names_from_file(smart_config, &selected_algos, SELECTED_ALGOS_FILENAME);

    algo_info_t filtered_out;
    filter_out_names_matching_regexes(&selected_algos, &filtered_out, algos, n_algos);

    if (filtered_out.num_algos > 0)
    {
        sort_algorithm_names(&filtered_out);
        print_algorithms_as_list("Removing algorithms ", &filtered_out);
        write_algo_names_to_file(smart_config, &selected_algos, SELECTED_ALGOS_FILENAME);
    }
    else
    {
        printf("No algorithms were found to remove from the existing set.\n");
    }
}

/*
 * Overwrites the current selected_algos file with the contents of a new file.
 */
void write_algo_names(const char *load_name, const char *save_name, const smart_config_t *smart_config)
{
    algo_info_t algorithms;
    read_algo_names_from_file(smart_config, &algorithms, load_name);
    write_algo_names_to_file(smart_config, &algorithms, save_name);
}

/*
 * Empties the selected_algos file.
 */
void empty_selected_algos(const smart_config_t *smart_config)
{
    char fullpath[MAX_PATH_LENGTH];
    set_full_path_or_exit(fullpath, smart_config->smart_config_dir, SELECTED_ALGOS_FILENAME);
    empty_file(fullpath);
}

/*
 * Prints any saved algo files in the smart config dir.
 */
void list_saved_algos(const smart_config_t *smart_config)
{
    char algo_files[MAX_SELECT_ALGOS][ALGO_NAME_LEN];
    int num_files = add_and_trim_filenames_with_suffix(algo_files, smart_config->smart_config_dir, 0, ".algos");
    if (num_files > 0)
    {
        printf("Saved algorithm sets in %s:\n", smart_config->smart_config_dir);
        for (int i = 0; i < num_files; i++)
        {
            printf("%s\n", algo_files[i]);
        }
    }
    else
    {
        printf("No saved algo files found at: %s\n", smart_config->smart_config_dir);
    }
}

/*
 * Prints the algorithms in the selected_algos file to the console, in a tabular format.
 */
void print_selected_algo_file(const smart_config_t *smart_config)
{
    algo_info_t algorithms;
    read_algo_names_from_file(smart_config, &algorithms, SELECTED_ALGOS_FILENAME);

    if (algorithms.num_algos > 0) {
        sort_algorithm_names(&algorithms);
        printf("Algorithms selected for benchmarking:\n");
        print_algorithms_in_tabular_format(&algorithms);
    }
    else
    {
        printf("No algorithms are selected for benchmarking, looked in %s/%s\n", smart_config->smart_config_dir, SELECTED_ALGOS_FILENAME);
    }
}

/*
 * Prints the algorithms in the named set file to the console, in a tabular format.
 */
void print_named_set(const smart_config_t *smart_config, const char *save_name)
{
    char filename_with_suffix[MAX_PATH_LENGTH];
    set_filename_suffix_or_exit(filename_with_suffix, save_name, ALGO_FILENAME_SUFFIX);

    algo_info_t algorithms;
    read_algo_names_from_file(smart_config, &algorithms, filename_with_suffix);

    if (algorithms.num_algos > 0) {
        sort_algorithm_names(&algorithms);
        printf("Algorithms in the named set: %s\n", filename_with_suffix);
        print_algorithms_in_tabular_format(&algorithms);
    }
    else
    {
        printf("No algorithms could be read from: %s\n", filename_with_suffix);
    }
}
/*
 * Prints all selectable algorithm shared objects in all the algo search paths defined in the config,
 * in a tabular format.
 */
void print_selectable_algos(const smart_config_t *smart_config) {

    algo_info_t algorithms;
    get_all_algo_names(smart_config, &algorithms);

    if (algorithms.num_algos > 0) {
        sort_algorithm_names(&algorithms);
        printf("\nAlgorithms available for benchmarking:\n");
        print_algorithms_in_tabular_format(&algorithms);
    }
    else
    {
        printf("No algorithms could be found to benchmark.");
    }
}

/*
 * Runs the correct select command given the select options.
 */
int exec_select(select_command_opts_t *opts, const smart_config_t *smart_config)
{
	switch (opts->select_command)
    {
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
        case DESELECT_ALL:
        {
            empty_selected_algos(smart_config);
            break;
        }
        case SHOW_ALL:
        {
            print_selectable_algos(smart_config);
            break;
        }
        case SHOW_SELECTED:
        {
            print_selected_algo_file(smart_config);
            break;
        }
        case SHOW_NAMED: {
            print_named_set(smart_config, opts->named_set);
            break;
        }
        case LIST_NAMED:
        {
            list_saved_algos(smart_config);
            break;
        }
        case SET_AS_DEFAULT:
        {
            char filename_with_suffix[MAX_PATH_LENGTH];
            set_filename_suffix_or_exit(filename_with_suffix,opts->named_set, ALGO_FILENAME_SUFFIX);
            write_algo_names(filename_with_suffix, SELECTED_ALGOS_FILENAME, smart_config);
            break;
        }
        case SAVE_AS:
        {
            char filename_with_suffix[MAX_PATH_LENGTH];
            set_filename_suffix_or_exit(filename_with_suffix, opts->named_set, ALGO_FILENAME_SUFFIX);
            write_algo_names(SELECTED_ALGOS_FILENAME, filename_with_suffix, smart_config);
            break;
        }
        default:
        {
            error_and_exit("Unknown select command enountered: %d\n", opts->select_command);
        }
    }

    return 0;
}
