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
 * If not configured otherwise by environment variables, smart will first obtain the location of the running
 * executable.  The smart bin folder itself can be called anything, as all other paths are determined relative
 * to that location assuming the structure above.
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
#include <unistd.h>
#include <sys/stat.h>
#include "whereami.c"
#include "utils.h"

#define MAX_SEARCH_PATHS 32

/*
 * Default directory names.
 */
#define SMART_DATA_DIR_DEFAULT "data"
#define SMART_ALGO_DIR_DEFAULT "algos"
#define SMART_RESULTS_PATH_DEFAULT "results"
#define SMART_CONFIG_PATH_DEFAULT "config"
#define SELECTED_ALGOS_FILENAME "selected_algos"

/*
 * Environment variable names to configure directories and search paths.
 */
#define SMART_DATA_DIR_ENV "SMART_DATA_DIR"
#define SMART_ALGO_DIR_ENV "SMART_ALGO_DIR"
#define SMART_RESULTS_DIR_ENV "SMART_RESULTS_DIR"
#define SMART_CONFIG_DIR_ENV "SMART_CONFIG_DIR"
#define SMART_DATA_SEARCH_PATHS_ENV "SMART_DATA_SEARCH_PATHS"
#define SMART_ALGO_SEARCH_PATHS_ENV "SMART_ALGO_SEARCH_PATHS"

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
 */
void set_smart_exe_dir(smart_config_t *config)
{
    char *path = NULL;
    int length, dirname_length;

    // Obtain executable path using whereami cross-platform library.
    // Usage is you first call it with null parameters in order to get the length required to allocate a buffer.
    // Then call it again with an appropriately sized buffer to get the actual path.
    length = wai_getExecutablePath(NULL, 0, &dirname_length);

    if (length >= MAX_PATH_LENGTH) {
        printf("\tWARN\tPath to executable is longer than max path length %d.\nYou may need to set paths to algorithms and data with environment variables.\n",
               MAX_PATH_LENGTH);
        config->smart_exe_dir[0] = '\0'; // no exe path obtained.
    }
    else {

        if (length > 0) {
            path = (char *) malloc(length + 1);
            if (path == NULL)
                abort();

            // Call again to obtain the actual path:
            wai_getExecutablePath(path, length, &dirname_length);
            path[length] = '\0';

            // We use length - 2 because length - 1 is the last character of the path.
            // If the last character of the path is already a slash, we want to ignore it and find the one before that.
            long last_slash = last_index_of_from_pos(path, '/', length - 2);

            memcpy(config->smart_exe_dir, path, last_slash);
            config->smart_exe_dir[last_slash] = '\0';
        } else {
            printf("\tWARN\tCould not obtain path to executable.\nYou may need to set paths to algorithms and data with environment variables.\n");
            config->smart_exe_dir[0] = '\0'; // no exe path obtained.
        }

        if (path != NULL)
            free(path);
    }
}

/*
 * Appends a folder to a path of a given length and places the result in string.
 * The path_len controls how much of the given path is used up.  Normally this will be the entire path string,
 * but you can use a shorter length so that you exclude some later directories on the path.
 * Ensures that all strings can be no longer than MAX_PATH_LENGTH.
 */
void append_folder(char *string, const char *path, long path_len, const char *folder)
{
    size_t folder_name_len = strlen(folder);
    int ends_with_slash = path[path_len - 1] == '/';
    int MAX_LEN = ends_with_slash ? MAX_PATH_LENGTH : MAX_PATH_LENGTH - 1;
    if (path_len + folder_name_len >= MAX_LEN)
    {
        printf("\tWARN\tPath to folder is longer than max path length %d:\n%s\\%s\n", MAX_PATH_LENGTH, path, folder);
        return;
    }

    if (ends_with_slash)
    {
        memcpy(string, path, path_len);
        strcpy(string + path_len, folder);
    }
    else
    {
        memcpy(string, path, path_len);
        string[path_len] = '/';
        strncpy(string + path_len + 1, folder, MAX_PATH_LENGTH);
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
        strncpy(search_paths[0], default_path, MAX_PATH_LENGTH);
        path_count++;
    }

    // Now add in any other search paths in the search path string, separated by :
    if (search_path != NULL)
    {
        const char path_delimitter = ':';
        size_t pathlen = strlen(search_path);
        char tokenised_search_path[pathlen + 1];
        strcpy(tokenised_search_path, search_path);

        char *path = strtok(tokenised_search_path, &path_delimitter);
        while (path != NULL && path_count < MAX_SEARCH_PATHS) {
            strcpy(search_paths[path_count++], path);
            path = strtok(NULL, &path_delimitter);
        }

        if (path_count >= MAX_SEARCH_PATHS && path != NULL)
        {
            printf("\tWARN\tMaximum number %d of %c separated search paths exceeded; ignoring any subsequent paths in:\n\t\t\t%s\n",
                   MAX_SEARCH_PATHS, path_delimitter, search_path);
        }
    }

    return path_count;
}

/*
 * Copies a path from src to dst, ensuring it is not longer than the MAX_PATH_LENGTH.
 * If it is longer than MAX_PATH_LENGTH, a warning message will be printed, and the string will be truncated
 * to MAX_PATH_LENGTH.
 */
void copy_path(char *dst, const char *src)
{
    size_t len = strlen(src);
    if (len >= MAX_PATH_LENGTH)
    {
        printf("\tWARN\tPath is longer than max path length:\n%s\n", src);
    }

    strncpy(dst, src, MAX_PATH_LENGTH);
}

/*
 * Sets a string to be either (1) the value of an environment variable if it is defined,
 * or (2) a path consisting of a root folder up to some length followed by a folder name.
 */
void set_env_var_or_default(char *string, const char *env_var_name, const char *root_folder, long root_path_len, const char *sub_folder)
{
    const char * env_var = getenv(env_var_name);
    if (env_var == NULL)
    {
        append_folder(string, root_folder, root_path_len, sub_folder);
    }
    else
    {
        copy_path(string, env_var);
    }
}

/*
 * Checks whether a path exists or not, and prints a warning message if it does not.
 * It attempts to use the current working directory instead in that case.
 * Returns true if the file exists (1) and false (0) if it does not.
 */
int check_exists_use_working_dir(char path[MAX_PATH_LENGTH], const char *description)
{
    int result = access(path, F_OK);
    if (result != 0)
    {
        printf("\tWARN\tFile path for %s is not accessible:\t%s\n", description, path);
        if (getcwd(path, MAX_PATH_LENGTH) != NULL)
        {
            printf("\t\t\tUsing the current working directory for %s files:\t%s\n", description, path);
        }
    }
    return result == 0;
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
    long root_path_len = strlen(config->smart_exe_dir);
    long last_slash = last_index_of_from_pos(config->smart_exe_dir, '/', root_path_len - 2);
    set_env_var_or_default(config->smart_config_dir, SMART_CONFIG_DIR_ENV, config->smart_exe_dir, last_slash, SMART_CONFIG_PATH_DEFAULT);
    check_exists_use_working_dir(config->smart_config_dir, "config");
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
    long root_path_len = strlen(config->smart_exe_dir);
    long last_slash = last_index_of_from_pos(config->smart_exe_dir, '/', root_path_len - 2);
    set_env_var_or_default(config->smart_results_dir, SMART_RESULTS_DIR_ENV, config->smart_exe_dir, last_slash, SMART_RESULTS_PATH_DEFAULT);
    check_exists_use_working_dir(config->smart_results_dir, "results");
}

/*
 * Sets the smart algo dir where the built shared objects of algorithms distributed with SMART are stored.
 * In order of precedence, this will be:
 * 1) A folder set by the environment variable SMART_ALGO_DIR.
 * 2) A default "/algos" folder, existing as a subfolder of the folder where the executable lives, i.e. /bin/algos
 */
void set_smart_algo_dir(smart_config_t *config)
{
    long root_path_len = strlen(config->smart_exe_dir);
    set_env_var_or_default(config->smart_algo_dir, SMART_ALGO_DIR_ENV, config->smart_exe_dir, root_path_len, SMART_ALGO_DIR_DEFAULT);
}

/*
 * Sets the smart data dir, where data which is distributed with SMART is stored.
 * In order of precedence, this will be:
 * 1) A folder set by the environment variable SMART_DATA_DIR
 * 2) A default "/data" folder, which is a peer folder of the /bin folder in which the executable lives.
 */
void set_smart_data_dir(smart_config_t *config)
{
    long root_path_len = strlen(config->smart_exe_dir);
    long last_slash = last_index_of_from_pos(config->smart_exe_dir, '/', root_path_len - 2);
    set_env_var_or_default(config->smart_data_dir, SMART_DATA_DIR_ENV, config->smart_exe_dir, last_slash, SMART_DATA_DIR_DEFAULT);
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
 * Builds a list of search paths for algos, given the smart algo dir folder (set by set_smart_algo_dir), and
 * the environment variable SMART_ALGO_SEARCH_PATHS, which is a : delimited string containing multiple paths to search in.
 */
void set_smart_algo_search_paths(smart_config_t *config)
{
    config->num_algo_search_paths = build_search_paths(config->smart_algo_dir,
                                                       getenv(SMART_ALGO_SEARCH_PATHS_ENV),
                                                       config->smart_algo_search_paths);
}

#endif //SMART_CONFIG_H
