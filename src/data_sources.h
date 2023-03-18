#ifndef SMART_DATA_SOURCES_H
#define SMART_DATA_SOURCES_H

#include "config.h"
#include "commands.h"

/*
 * Computes information about the size of the alphabet contained in character frequency table freq, and gives
 * both the number of unique characters, and the maximum character code it contains.
 */
void compute_alphabet_info(const int *freq, int *alphabet_size, int *max_code)
{
    *alphabet_size = 0;
    *max_code = 0;
    for (int c = 0; c < SIGMA; c++)
    {
        if (freq[c] != 0)
        {
            (*alphabet_size)++;

            if (c > *max_code)
                *max_code = c;
        }
    }
}

/*
 * Computes the frequency of characters contained in text buffer T of size n, and places them in the freq table.
 */
void compute_frequency(const unsigned char *T, int n, int *freq)
{
    for (int j = 0; j < SIGMA; j++)
        freq[j] = 0;

    for (int j = 0; j < n; j++)
        freq[T[j]]++;
}

/*
 * Loads the files defined in the list of filenames into a text buffer T, up to a maximum size of text.
 * Returns the current size of data loaded.
 */
int merge_text_buffers(const char filenames[][MAX_PATH_LENGTH], int n_files, unsigned char *T, int max_text_size)
{
    int curr_size = 0;
    for (int i = 0; i < n_files && curr_size < max_text_size; i++)
    {
        info("Loading the file %s", filenames[i]);
        int size = load_text_buffer(filenames[i], T + curr_size, max_text_size - curr_size);

        if (size > 0)
        {
            curr_size += size;
        }
        else
        {
            warn("Could not load file: %s", filenames[i]);
        }
    }

    return curr_size;
}

/*
 * Replicates existing data of length size in a buffer to fill up any remaining space in the buffer.
 */
void replicate_buffer(unsigned char *buffer, int size, int target_size)
{
    if (size > 0)
    {
        while (size < target_size)
        {
            int cpy_size = (target_size - size) < size ? (target_size - size) : size;
            memcpy(buffer + size, buffer, cpy_size);
            size += cpy_size;
        }
    }
}

/*
 * Adds a file or the files in a directory defined in the data source to a list of filenames,
 * searching for them in the search paths defined.
 * If the data source points to a directory, all the files in that directory are added.
 * If the data source points to a file, just that file is added.
 * Returns the number of files in the filenames array.
 */
int add_files(const char search_paths[][MAX_PATH_LENGTH], int num_search_paths, const char *data_source,
              char filenames[][MAX_PATH_LENGTH], int num_files, int max_files)
{
    char valid_path[MAX_PATH_LENGTH];
    locate_file_path(valid_path, data_source, search_paths, num_search_paths);

    if (!is_empty_string(valid_path))
    {
        __mode_t file_mode = get_file_mode(valid_path);
        if (S_ISDIR(file_mode))
        {
            num_files = add_filenames_in_dir(valid_path, filenames, num_files, max_files);
        }
        else if (S_ISREG(file_mode) && num_files < max_files)
        {
            strcpy(filenames[num_files++], valid_path);
        }
    }

    return num_files;
}

/*
 * Builds a list of filenames from the data sources and places them in filenames, searching the search path.
 * Returns the number of files in the filenames array.
 */
int build_list_of_files_to_load(const smart_config_t *smart_config, const char *data_sources[MAX_DATA_SOURCES], char filenames[][MAX_PATH_LENGTH])
{
    int source_index = 0, num_files = 0;
    while (source_index < MAX_DATA_SOURCES && num_files < MAX_DATA_FILES && data_sources[source_index] != NULL)
    {
        num_files = add_files(smart_config->smart_data_search_paths,
                              smart_config->num_data_search_paths,
                              data_sources[source_index++],
                              filenames, num_files, MAX_DATA_FILES);
    }

    return num_files;
}

/*
 * Generates a random text and stores it in the buffer of size bufsize, with an alphabet of sigma.
 * Returns the size of the random data (which will be bufsize).
 */
int gen_random_text(const int sigma, unsigned char *buffer, const int bufsize)
{
    // An alphabet of one means all symbols are the same - so just set zero.
    if (sigma == 1)
    {
        memset(buffer, 0, bufsize);
    }
    else
    {
        for (int i = 0; i < bufsize; i++)
        {
            buffer[i] = rand() % sigma;
        }
    }
    return bufsize;
}

/*
 * Loads all the files located in an array of data_sources into a buffer up to the bufsize.
 * Uses the search path to locate files or directories if they do not exist locally.
 * Returns the size of data loaded into the buffer.
 */
int gen_search_text(const smart_config_t *smart_config, const char *data_sources[MAX_DATA_SOURCES], unsigned char *buffer, int bufsize, int fill_buffer)
{
    char filenames[MAX_DATA_FILES][MAX_PATH_LENGTH];
    int n_files = build_list_of_files_to_load(smart_config, data_sources, filenames);
    if (n_files > 0)
    {
        int n = merge_text_buffers(filenames, n_files, buffer, bufsize);

        if (n >= bufsize || !fill_buffer)
            return n;

        replicate_buffer(buffer, n, bufsize);

        if (n > 0)
            return bufsize;
    }

    error_and_exit("No files could be found to generate the search text.");
}

/*
 * Loads user supplied data from the command line as the search data into the buffer.
 *
 * WARNING: the buffer must be guaranteed to be at least opts->text_size in size.
 */
int gen_user_data(const run_command_opts_t *opts, unsigned char *buffer)
{
    int size = strlen(opts->data_to_search);
    int to_copy = size < opts->text_size ? size : opts->text_size;
    memcpy(buffer, opts->data_to_search, to_copy);

    return size;
}


#endif //SMART_DATA_SOURCES_H
