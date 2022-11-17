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

#include <stdlib.h>
#include <string.h>
#include "whereami.c"

//TODO: check existence of paths determine?  Abort with error message if paths cannot be found?

#define MAX_PATH_LENGTH 2048
#define MAX_SEARCH_PATHS 16

/*
 * Default directory names.
 */
#define SMART_DATA_DIR_DEFAULT "data"
#define SMART_ALGO_DIR_DEFAULT "algos"
#define RESULTS_ROOT_PATH "~"
#define SMART_RESULTS_PATH_DEFAULT "smart-search/results"
#define CONFIG_ROOT_PATH "~"
#define SMART_CONFIG_PATH_DEFAULT ".config/smart-search"

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
    set_smart_algo_search_paths(config);   // This depends on smart_algo_dir
    set_smart_data_dir(config);            // This depends on smart_exe_dir.
    set_smart_data_search_paths(config);   // This depends on smart_data_dir.
}

int last_index_of_from_pos(const char *string, char to_find, int pos)
{
    int limit = strlen(string) - 1;
    if (pos > limit) pos = limit;
    while (pos >= 0 && string[pos] != to_find) pos--;
    return pos;
}

void set_smart_exe_dir(smart_config_t *config)
{
    char *path = NULL;
    int length, dirname_length;

    length = wai_getExecutablePath(NULL, 0, &dirname_length);

    if (length >= MAX_PATH_LENGTH) {
        printf("ERROR - path to exe is longer than max path length %d.", MAX_PATH_LENGTH);
        exit(1);
    }

    if (length > 0)
    {
        path = (char *) malloc(length + 1);
        if (path == NULL)
            abort();
        wai_getExecutablePath(path, length, &dirname_length);
        path[length] = '\0';

        // We use length - 2 because length - 1 is the last character of the path.
        // If the last character of the path is already a slash, we want to ignore it and find the one before that.
        int last_slash = last_index_of_from_pos(path, '/', length - 2);

        memcpy(config->smart_exe_dir, path, last_slash);
        config->smart_exe_dir[last_slash] = '\0';
    }
    else
    {
        config->smart_exe_dir[0] = '\0'; // no exe path obtained - should we quit?
    }

    if (path != NULL)
        free(path);
}

void append_folder(char *string, const char *path, int path_len, const char *folder)
{
    int folder_name_len = strlen(folder);
    int ends_with_slash = path[path_len - 1] == '/';
    int MAX_LEN = ends_with_slash ? MAX_PATH_LENGTH : MAX_PATH_LENGTH - 1;
    if (path_len + folder_name_len >= MAX_LEN)
    {
        printf("ERROR - path to exe is longer than max path length %d:\n%s\\%s\n", MAX_PATH_LENGTH, path, folder);
        exit(1);
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

void copy_path(char *dst, const char *src)
{
    int len = strlen(src);
    if (len >= MAX_PATH_LENGTH)
    {
        printf("ERROR - path to exe is longer than max path length:\n%s\n", src);
        exit(1);
    }

    strncpy(dst, src, MAX_PATH_LENGTH);
}

void set_env_var_or_default(char *string, const char *env_var_name, const char *root_folder, int root_path_len, const char *default_dir)
{
    const char * env_var = getenv(env_var_name);
    if (env_var == NULL)
    {
        append_folder(string, root_folder, root_path_len, default_dir);
    }
    else
    {
        copy_path(string, env_var);
    }
}

void set_smart_config_dir(smart_config_t *config)
{
    int root_path_len = strlen(CONFIG_ROOT_PATH);
    set_env_var_or_default(config->smart_config_dir, SMART_CONFIG_DIR_ENV, CONFIG_ROOT_PATH, root_path_len, SMART_CONFIG_PATH_DEFAULT);
}

void set_smart_results_dir(smart_config_t *config)
{
    int root_path_len = strlen(RESULTS_ROOT_PATH);
    set_env_var_or_default(config->smart_results_dir, SMART_RESULTS_DIR_ENV, RESULTS_ROOT_PATH, root_path_len, SMART_RESULTS_PATH_DEFAULT);
}

void set_smart_algo_dir(smart_config_t *config)
{
    int root_path_len = strlen(config->smart_exe_dir);
    set_env_var_or_default(config->smart_algo_dir, SMART_ALGO_DIR_ENV, config->smart_exe_dir, root_path_len, SMART_ALGO_DIR_DEFAULT);
}

void set_smart_data_dir(smart_config_t *config)
{
    int root_path_len = strlen(config->smart_exe_dir);
    int last_slash = last_index_of_from_pos(config->smart_exe_dir, '/', root_path_len - 2);
    set_env_var_or_default(config->smart_data_dir, SMART_DATA_DIR_ENV, config->smart_exe_dir, last_slash, SMART_DATA_DIR_DEFAULT);
}


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
        const char *path_delimitter = ":";
        int pathlen = strlen(search_path);
        char tokenised_search_path[pathlen + 1];
        strcpy(tokenised_search_path, search_path);

        char *path = strtok(tokenised_search_path, path_delimitter);
        while (path != NULL && path_count < MAX_PATH_LENGTH) {
            strcpy(search_paths[path_count++], path);
            path = strtok(NULL, path_delimitter);
        }
    }

    return path_count;
}

void set_smart_data_search_paths(smart_config_t *config)
{
    config->num_data_search_paths = build_search_paths(config->smart_data_dir,
                                             getenv(SMART_DATA_SEARCH_PATHS_ENV),
                                             config->smart_data_search_paths);
}

void set_smart_algo_search_paths(smart_config_t *config)
{
    config->num_algo_search_paths = build_search_paths(config->smart_algo_dir,
                                                       getenv(SMART_ALGO_SEARCH_PATHS_ENV),
                                                       config->smart_algo_search_paths);
}

#endif //SMART_CONFIG_H
