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
 * contact the authors at: faro@dmi.unict.it and thierry.lecroq@univ-rouen.fr
 * download the tool at: http://www.dmi.unict.it/~faro/smart/
 */
#ifndef SMART_CONFIG_H
#define SMART_CONFIG_H

/*
 * This unit defines the file paths that smart uses to locate the files it needs.
 * The default folder structure of smart is as follows, assuming a "smart" parent folder:
 *
 * smart/bin         location of the smart executable file
 * smart/bin/algos   location of the compiled algorithm shared objects.
 * smart/data        location of built-in data to search with.
 * smart/config      location of config files, such as which algorithms are selected and saved algorithm sets.
 * smart/results     location where smart writes out results by default.
 *
 * smart will first obtain the location of the running executable.
 * The smart folder, and smart bin folder itself can be called anything, as all other paths are determined relative
 * to that location, assuming the structure above.
 *
 * All the paths above (except the smart/bin folder determined at runtime) can be configured be setting
 * the appropriate environment variable.  If set, it will override the structure above for that item.
 *
 * smart/bin/algos  SMART_ALGO_DIR
 * smart/data       SMART_DATA_DIR
 * smart/config     SMART_CONFIG_DIR
 * smart/results    SMART_RESULTS_DIR
 *
 * You can also configure additional search paths for data and algorithms that smart will use to locate them.
 * These are unix-style search paths, with each path separated by a colon :
 *
 * Data                        SMART_DATA_SEARCH_PATHS
 * Algorithm shared objects    SMART_ALGO_SEARCH_PATHS
 *
 * Search paths are set in addition to the paths smart sets for data or algos, so any default path (or path set by
 * environment variable) will still be searched as well as any paths defined in the search path environment variable.
 */

#include <stdlib.h>
#include <string.h>

#include "whereami.c"
#include "defines.h"
#include "utils.h"

/*
 * Paths to the locations of files required by smart and search paths to locate user data or algorithms.
 */
typedef struct smart_config
{
    char smart_exe_dir[MAX_PATH_LENGTH];                              // location of executable.
    char smart_config_dir[MAX_PATH_LENGTH];                           // location of selected_algos and any other user config.
    char smart_results_dir[MAX_PATH_LENGTH];                          // location where smart experiment results are stored.
    char smart_algo_dir[MAX_PATH_LENGTH];                             // location of default smart algorithms.
    int num_algo_search_paths;                                        // number of search paths for algorithms.
    char smart_algo_search_paths[MAX_SEARCH_PATHS][MAX_PATH_LENGTH];  // search paths for smart algorithms and user algorithms.
    char smart_data_dir[MAX_PATH_LENGTH];                             // location of default smart data corpora.
    int num_data_search_paths;                                        // number of search paths for data
    char smart_data_search_paths[MAX_SEARCH_PATHS][MAX_PATH_LENGTH];  // search paths for smart and user corpora.
} smart_config_t;

/*
 * Forward declarations of methods.
 */
void set_smart_exe_dir(smart_config_t *config);
void set_smart_config_dir(smart_config_t *config);
void set_smart_results_dir(smart_config_t *config);
void set_smart_algo_dir(smart_config_t *config);
void set_smart_algo_search_paths(smart_config_t *config);
void set_smart_data_dir(smart_config_t *config);
void set_smart_data_search_paths(smart_config_t *config);

/*
 * Locates file paths required by SMART to function, and parses any environment variables such as search paths.
 */
void init_config(smart_config_t *config)
{
    set_smart_exe_dir(config);             // This must be processed first as other directories depend on it.
    set_smart_config_dir(config);          // This is just relative to user home directory.
    set_smart_results_dir(config);         // This is just relative to user home directory.
    set_smart_algo_dir(config);            // This depends on smart_exe_dir.
    set_smart_algo_search_paths(config);   // This uses smart_algo_dir if it exists - but also set with environment variable.
    set_smart_data_dir(config);            // This depends on smart_exe_dir.
    set_smart_data_search_paths(config);   // This uses smart_data_dir if it exists - but also set with environment variable.
}

/*
 * Sets the path to the currently running SMART executable, using the whereami library to get this in a cross platform way.
 *
 * Usage of whereami is to first call with NULL buffer to get the size of the buffer to allocate.
 * Then call again with allocated buffer to get the actual path.
 */
void set_smart_exe_dir(smart_config_t *config)
{
    int length, dirname_length;
    length = wai_getExecutablePath(NULL, 0, &dirname_length);

    if (length > 0)
    {
        // Obtain the exe path, which includes the name of the executable.
        char * path = (char *) malloc(length + 1);
        wai_getExecutablePath(path, length, &dirname_length);
        path[length] = STR_END_CHAR; // ensure path is fully terminated.

        // Get the folder the exe is in by stripping back to the last /
        char folder_without_exe_name[MAX_PATH_LENGTH];
        set_one_folder_back_or_exit(folder_without_exe_name, path);
        copy_path_or_exit(config->smart_exe_dir, folder_without_exe_name);

        free(path);
    }
    else
    {
        warn("Could not obtain path to executable.\nYou may need to set paths to algorithms and data with environment variables.");
        set_empty_string(config->smart_exe_dir); // no exe path obtained.  //TODO: test what happens if no exe path is obtained.
    }
}

/*
 * Builds a list of search paths given a default path which will be the first entry in the list if not NULL,
 * and a search path, which consists of one or more paths in a single string separated by colon delimiters :
 */
int build_search_paths(const char *default_path, const char *search_path, char search_paths[][MAX_PATH_LENGTH])
{
    int path_count = 0;

    // Make the default path the first search path, if it exists.
    if (default_path != NULL) {
        copy_path_or_exit(search_paths[path_count++], default_path);
    }

    // Now add in any other search paths in the search path string, separated by :
    if (search_path != NULL)
    {
        size_t pathlen = strlen(search_path);
        char tokenised_search_path[pathlen + 1];
        strcpy(tokenised_search_path, search_path);

        char *path = strtok(tokenised_search_path, SEARCH_PATH_DELIMITER);
        while (path != NULL && path_count < MAX_SEARCH_PATHS) {
            copy_path_or_exit(search_paths[path_count++], path);
            path = strtok(NULL, SEARCH_PATH_DELIMITER);
        }

        if (path_count >= MAX_SEARCH_PATHS && path != NULL)
        {
            warn("Maximum number %d of %c delimited search paths exceeded; ignoring subsequent paths in:\n%s",
                 MAX_SEARCH_PATHS, SEARCH_PATH_DELIMITER, search_path);
        }
    }

    return path_count;
}

/*
 * Checks whether a path is accessible or not, and prints a warning message if it does not.
 * It attempts to use the current working directory instead in that case.
 * If it can't fall back to the current working directory, it makes the path empty.
 */
void fallback_to_working_dir_if_not_accessible(char path[MAX_PATH_LENGTH], const char *description)
{
    if (access(path, F_OK) != 0)
    {
        warn("File path for %s files is not accessible:\t%s", description, path);

        if (getcwd(path, MAX_PATH_LENGTH) != NULL)
        {
            warn("Using the current working directory for %s files:\t%s", description, path);
        }
        else
        {
            warn("Could not obtain current working directory for %s files, empty dir used.", description);
            set_empty_string(path);
        }
    }
}

/*
 * Sets the smart algo dir where the built shared objects of algorithms distributed with SMART are stored.
 * In order of precedence, this will be:
 * 1) A folder set by the environment variable SMART_ALGO_DIR.
 * 2) A default "/algos" folder, existing as a subfolder of the folder where the executable lives, i.e. /bin/algos
 *
 * This does not fall back to the current working directory if it is not accessible.  It is not likely that the algo
 * shared objects will be found there if the other directories are not accessible.
 *
 * Operations that involve looking at the algo shared objects (listing, benchmarking and testing) will not succeed.
 */
void set_smart_algo_dir(smart_config_t *config)
{
    char default_path[MAX_PATH_LENGTH];
    set_full_path_or_exit(default_path, config->smart_exe_dir, SMART_ALGO_DIR_DEFAULT);
    set_env_var_or_default(config->smart_algo_dir, SMART_ALGO_DIR_ENV, default_path);
}

/*
 * Sets a path from an environment variable if set, or by setting as a peer folder of another.
 */
void set_path_from_env_var_or_peer_folder(char *path_to_set, const char *env_var_name, const char *peer_folder, const char *folder_name, const char *description)
{
    char one_folder_back[MAX_PATH_LENGTH];
    set_one_folder_back_or_exit(one_folder_back, peer_folder);

    char default_path[MAX_PATH_LENGTH];
    set_full_path_or_exit(default_path, one_folder_back, folder_name);

    set_env_var_or_default(path_to_set, env_var_name, default_path);
    fallback_to_working_dir_if_not_accessible(path_to_set, description);
}

/*
 * Sets the smart config dir that contains the selected_algos file and any other user-specific configuration.
 * In order of precedence, this will be:
 * 1) A folder set by the environment variable SMART_CONFIG_DIR.
 * 2) A "/config" folder which is a peer folder of the /bin folder in which the executable lives.
 * 3) The current working folder, if none of those are set or exist.
 */
void set_smart_config_dir(smart_config_t *config)
{
    set_path_from_env_var_or_peer_folder(config->smart_config_dir, SMART_CONFIG_DIR_ENV,
                                         config->smart_exe_dir, SMART_CONFIG_PATH_DEFAULT, "config");
}

/*
 * Sets the smart results dir in which any experimental results are saved.
 * In order of precedence, this will be:
 * 1) A folder set by the environment variable SMART_RESULTS_DIR.
 * 2) A "/results" folder which is a peer folder of the /bin folder in which the executable lives.
 * 3) The current working folder, if none of those are set or exist.
 */
void set_smart_results_dir(smart_config_t *config)
{
    set_path_from_env_var_or_peer_folder(config->smart_results_dir, SMART_RESULTS_DIR_ENV,
                                         config->smart_exe_dir, SMART_RESULTS_PATH_DEFAULT, "results");
}

/*
 * Sets the smart data dir, where data which is distributed with SMART is stored.
 * In order of precedence, this will be:
 * 1) A folder set by the environment variable SMART_DATA_DIR
 * 2) A default "/data" folder, which is a peer folder of the /bin folder in which the executable lives.
 */
void set_smart_data_dir(smart_config_t *config)
{
    set_path_from_env_var_or_peer_folder(config->smart_data_dir, SMART_DATA_DIR_ENV,
                                         config->smart_exe_dir, SMART_DATA_DIR_DEFAULT, "data");
}

/*
 * Builds a list of search paths for data, given the smart data dir folder (set by set_smart_data_dir), and
 * the environment variable SMART_DATA_SEARCH_PATHS, which is a : delimited string containing multiple paths to search in.
 */
void set_smart_data_search_paths(smart_config_t *config)
{
    config->num_data_search_paths = build_search_paths(config->smart_data_dir,
                                             getenv(SMART_DATA_SEARCH_PATHS_ENV),
                                             config->smart_data_search_paths);
}

/*
 * Builds a list of search paths for algo_names, given the smart algo dir folder (set by set_smart_algo_dir), and
 * the environment variable SMART_ALGO_SEARCH_PATHS, which is a : delimited string containing multiple paths to search in.
 */
void set_smart_algo_search_paths(smart_config_t *config)
{
    config->num_algo_search_paths = build_search_paths(config->smart_algo_dir,
                                                       getenv(SMART_ALGO_SEARCH_PATHS_ENV),
                                                       config->smart_algo_search_paths);
}

/*
 * Prints the smart configuration values.
 */
void print_config(smart_config_t *config)
{
    print_logo();

    printf("\nPaths determined:\n\n");

    print_name_value("Executable path:", config->smart_exe_dir, COL_WIDTH);
    print_name_value("Config path:", config->smart_config_dir, COL_WIDTH);
    print_name_value("Results path:", config->smart_results_dir, COL_WIDTH);
    print_list_of_paths("Algorithm path(s):", config->smart_algo_search_paths,
                        config->num_algo_search_paths, COL_WIDTH);
    print_list_of_paths("Data path(s)", config->smart_data_search_paths,
                        config->num_data_search_paths, COL_WIDTH);
    print_file_and_access("Selected algorithms file:", config->smart_config_dir, SELECTED_ALGOS_FILENAME, COL_WIDTH);

    printf("\nEnvironment variables:\n\n");

    print_env_var(SMART_CONFIG_DIR_ENV, COL_WIDTH);
    print_env_var(SMART_ALGO_DIR_ENV, COL_WIDTH);
    print_env_var(SMART_RESULTS_DIR_ENV, COL_WIDTH);
    print_env_var(SMART_ALGO_SEARCH_PATHS_ENV, COL_WIDTH);
    print_env_var(SMART_DATA_SEARCH_PATHS_ENV, COL_WIDTH);

    printf("\n");
}



#endif //SMART_CONFIG_H
