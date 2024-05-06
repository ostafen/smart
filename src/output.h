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

#include "config.h"
#include "utils.h"
#include "commands.h"
#include "bench_results.h"
#include "algos/include/stats.h"

static char *const NUM_ALGORITHMS_BENCHMARKED_KEY = "Num algorithms benchmarked";
static char *const ALGORITHM_BENCHMARKED_KEY = "Algorithm benchmarked";
static char *const TEXT_LENGTH_KEY = "Text length";
static char *const TEXT_MIN_CHAR_CODE = "Text minimum character code";
static char *const TEXT_MAX_CHAR_CODE = "Text maximum character code";
static char *const TEXT_SHANNON_ENTROPY = "Text shannon entropy";
static char *const STARTED_BENCHMARKING = "Started benchmarking";
static char *const FINISHED_BENCHMARKING = "Finished benchmarking";

static const int COLOR_STRING_LENGTH = 22;

enum measurement_unit {MILLISECONDS, GIGABYTES_PER_SECOND};


const char *unit_description(enum measurement_unit unit)
{
    return unit == MILLISECONDS ? "Results in ms." : "Results in Gb/s.";
}

/*
 * Sets the filename of an experiment using the experiment code, an optional description, and a suffix.
 */
void set_experiment_filename(const run_command_opts_t *opts, char file_name[MAX_PATH_LENGTH], const char *output_type, const char *suffix)
{
    if (opts->description)
    {
        snprintf(file_name, MAX_PATH_LENGTH, "%s - %s - %s.%s", opts->expcode, opts->description, output_type, suffix);
    }
    else
    {
        snprintf(file_name, MAX_PATH_LENGTH, "%s - %s.%s", opts->expcode, output_type, suffix);
    }
}

/*
 * Opens an experiment file for writing with the given description and suffix.
 */
FILE *open_experiment_file_for_writing(const smart_config_t *smart_config, const run_command_opts_t *opts,
                                       const char *description, const char *suffix)
{
    char filename[MAX_PATH_LENGTH];
    set_experiment_filename(opts, filename, description, suffix);

    char full_path[MAX_PATH_LENGTH];
    set_full_path_or_exit(full_path, smart_config->smart_results_dir, filename);

    return fopen(full_path, "w");
}


/*
 * Sets a description of the data source for the search text.
 */
void set_data_source_description(const run_command_opts_t *opts, char description[MAX_LINE_LEN])
{
    switch (opts->data_source)
    {
        case DATA_SOURCE_FILES:
        {
            description[0] = STR_END_CHAR;
            for (int i = 0; i < MAX_DATA_SOURCES && opts->data_sources[i] != NULL; i++)
            {
                if (i > 0) strcat(description, ", ");
                strcat(description, opts->data_sources[i]);
            }
            break;
        }
        case DATA_SOURCE_RANDOM:
        {
            snprintf(description, MAX_LINE_LEN, "Random alphabet %d", opts->alphabet_size);
            break;
        }
        case DATA_SOURCE_USER:
        {
            snprintf(description, MAX_LINE_LEN, "%s", "Command line");
            break;
        }
        default: {
            snprintf(description, MAX_LINE_LEN, "%s", "ERROR: data source not defined.");
        }
    }
}

/*
 * Repeats a string a number of times separated by tab characters.
 * This is useful to write rows of a tab separated CSV file with the same value (e.g. "OUT").
 */
void write_tabbed_string(FILE *fp, const char *string, int num_repetitions)
{
    if (num_repetitions > 0)
    {
        fprintf(fp, "%s", string);
        for (int i = 1; i < num_repetitions; i++)
        {
            fprintf(fp, "\t%s", string);
        }
    }
}

/*
 * Writes out the summary of the experiment run to a text file.
 */
void output_benchmark_run_summary(const smart_config_t *smart_config, const run_command_opts_t *opts,
                                  const algo_info_t *algorithms)
{
    FILE *sf = open_experiment_file_for_writing(smart_config, opts, "experiment", "txt");

    save_run_options(sf, opts);

    fprintf(sf, INT_KEY_FMT, TEXT_LENGTH_KEY, opts->text_stats.text_actual_length);
    fprintf(sf, INT_KEY_FMT, TEXT_MIN_CHAR_CODE, opts->text_stats.text_smallest_character_code);
    fprintf(sf, INT_KEY_FMT, TEXT_MAX_CHAR_CODE, opts->text_stats.text_greater_character_code);
    fprintf(sf, DOUBLE_KEY_FMT, TEXT_SHANNON_ENTROPY, opts->precision, opts->text_stats.shannon_entropy_byte);

    fprintf(sf, INT_KEY_FMT, NUM_ALGORITHMS_BENCHMARKED_KEY, algorithms->num_algos);
    for (int i = 0; i < algorithms->num_algos; i++)
    {
        fprintf(sf, STR_KEY_FMT, ALGORITHM_BENCHMARKED_KEY, algorithms->algo_names[i]);
    }

    if (opts->pinnned_cpu >= 0)
        fprintf(sf, INT_KEY_FMT, PINNED_CPU_KEY, opts->pinnned_cpu);
    else
        fprintf(sf, STR_KEY_FMT, PINNED_CPU_KEY, "not pinned");

    char time_string[TIME_FORMAT_STRLEN];
    set_time_string_with_time(time_string, TIME_FORMAT_STRLEN, TIME_FORMAT, opts->started_date);
    fprintf(sf, STR_KEY_FMT, STARTED_BENCHMARKING, time_string);

    
    set_time_string_with_time(time_string, TIME_FORMAT_STRLEN, TIME_FORMAT, opts->finished_date);
    fprintf(sf, STR_KEY_FMT, FINISHED_BENCHMARKING, time_string);
    
    fclose(sf);
}

/*
 * Outputs benchmark measurements as a tab separated CSV file.
 */
void output_algorithm_measurements_csv(const smart_config_t *smart_config, const run_command_opts_t *opts, int num_pattern_lengths,
                                       benchmark_results_t *results, const algo_info_t *algorithms)
{
    const int MEASUREMENT_COLUMNS = 9 + NUM_EXTRA_FIELDS;

    FILE *rf = open_experiment_file_for_writing(smart_config, opts, "algo-measurements", "csv");

    fprintf(rf, "EXPERIMENT\tPLEN\tALGORITHM\tMeasurement");
    fprintf(rf, "\t%% Text read\tAvg jump\tText bytes read\tPattern bytes read\t#Computations\t#Writes\t#Branches\t#Jumps\t#Lookups\t#Verifications");
    for (int i = 0; i < NUM_EXTRA_FIELDS; i++) {
        fprintf(rf, "\tExtra data %d", i);
    }
    fprintf(rf, "\n");

    // For each pattern length benchmarked:
    for (int pattern_len_no = 0; pattern_len_no < num_pattern_lengths; pattern_len_no++)
    {
        int pat_len = results[pattern_len_no].pattern_length;

        // For each algorithm:
        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            char upper_case_name[ALGO_NAME_LEN];
            set_upper_case_algo_name(upper_case_name, algorithms->algo_names[algo_no]);

            algo_results_t *algo_res = &(results[pattern_len_no].algo_results[algo_no]);
            switch (algo_res->success_state)
            {
                case SUCCESS:
                {
                    algo_measurements_t *measurements = &(algo_res->measurements);
                    for (int measurement = 0; measurement < opts->num_runs; measurement++)
                    {
                        fprintf(rf, "%s\t%d\t%s\t%d\t", opts->expcode, pat_len, upper_case_name, measurement);

                        fprintf(rf, "%.*f\t%.*f\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld",
                                opts->precision, (double) measurements->algo_stats[measurement].text_bytes_read / opts->num_runs /
                                                  opts->text_stats.text_actual_length * 100,
                                0, (double) (opts->text_stats.text_actual_length - pat_len) /
                                            ((double) measurements->algo_stats[measurement].num_jumps / opts->num_runs),
                                measurements->algo_stats[measurement].text_bytes_read,
                                measurements->algo_stats[measurement].pattern_bytes_read,
                                measurements->algo_stats[measurement].num_computations,
                                measurements->algo_stats[measurement].num_writes,
                                measurements->algo_stats[measurement].num_branches,
                                measurements->algo_stats[measurement].num_jumps,
                                measurements->algo_stats[measurement].num_lookups,
                                measurements->algo_stats[measurement].num_verifications);
                        for (int i = 0; i < NUM_EXTRA_FIELDS; i++)
                        {
                            fprintf(rf, "\t%ld", measurements->algo_stats[measurement].extra[i]);
                        }
                        fprintf(rf, "\n");
                    }

                    break;
                }
                case CANNOT_SEARCH:
                {
                    fprintf(rf, "%s\t%d\t%s\t", opts->expcode, pat_len, upper_case_name);
                    write_tabbed_string(rf, "---", MEASUREMENT_COLUMNS);
                    fprintf(rf, "\n");
                    break;
                }
                case TIMED_OUT:
                {
                    fprintf(rf, "%s\t%d\t%s\t", opts->expcode, pat_len, upper_case_name);
                    write_tabbed_string(rf, "OUT", MEASUREMENT_COLUMNS);
                    fprintf(rf, "\n");
                    break;
                }
                case ERROR:
                {
                    fprintf(rf, "%s\t%d\t%s\t", opts->expcode, pat_len, upper_case_name);
                    write_tabbed_string(rf, "ERROR", MEASUREMENT_COLUMNS);
                    fprintf(rf, "\n");
                    break;
                }
            }
        }
    }

    fclose(rf);
}

/*
 * Outputs performance measurements as a tab separated CSV file.
 */
void output_performance_measurements_csv(const smart_config_t *smart_config, const run_command_opts_t *opts, int num_pattern_lengths,
                                       benchmark_results_t *results, const algo_info_t *algorithms)
{
    const int MEASUREMENT_COLUMNS = 7;
    const int CPU_STAT_COLUMNS = 6;
    const int TOTAL_COLUMNS = MEASUREMENT_COLUMNS + CPU_STAT_COLUMNS;

    FILE *rf = open_experiment_file_for_writing(smart_config, opts, "algo-measurements", "csv");

    //TODO: output algo statistic measurements here too, or in another file?

    fprintf(rf, "EXPERIMENT\tPLEN\tALGORITHM");
    fprintf(rf, "\tMEASUREMENT\tPRE TIME (ms)\tSEARCH TIME (ms)\tTOTAL TIME (ms)\tPRE TIME (Gb/s)\tSEARCH TIME (Gb/s)\tTOTAL TIME (Gb/s)");
    fprintf(rf, opts->cpu_stats ? "\tL1_CACHE_ACCESS\tL1_CACHE_MISSES\tLL_CACHE_ACCESS\tLL_CACHE_MISSES\tBRANCH INSTRUCTIONS\tBRANCH MISSES\n" : "\n");

    // For each pattern length benchmarked:
    for (int pattern_len_no = 0; pattern_len_no < num_pattern_lengths; pattern_len_no++)
    {
        int pat_len = results[pattern_len_no].pattern_length;

        // For each algorithm:
        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            char upper_case_name[ALGO_NAME_LEN];
            set_upper_case_algo_name(upper_case_name, algorithms->algo_names[algo_no]);

            algo_results_t *algo_res = &(results[pattern_len_no].algo_results[algo_no]);
            switch (algo_res->success_state)
            {
                case SUCCESS:
                {
                    algo_measurements_t *measurements = &(algo_res->measurements);
                    for (int measurement = 0; measurement < opts->num_runs; measurement++)
                    {
                        fprintf(rf, "%s\t%d\t%s\t%d\t", opts->expcode, pat_len, upper_case_name, measurement);
                        fprintf(rf, "%f\t%f\t%f\t%f\t%f\t%f",
                                measurements->pre_times[measurement],
                                measurements->search_times[measurement],
                                measurements->pre_times[measurement] + measurements->search_times[measurement],
                                GBs(measurements->pre_times[measurement], pat_len),
                                GBs(measurements->search_times[measurement], opts->text_stats.text_actual_length),
                                GBs(measurements->pre_times[measurement] + measurements->search_times[measurement], pat_len + opts->text_stats.text_actual_length));
                        if (opts->cpu_stats)
                        {
                            if (opts->cpu_stats & CPU_STAT_L1_CACHE)
                                fprintf(rf, "\t%lld\t%lld",
                                        measurements->cpu_stats[measurement].l1_cache_access,
                                        measurements->cpu_stats[measurement].l1_cache_misses);
                            else
                                fprintf(rf, "\t---\t---");

                            if (opts->cpu_stats & CPU_STAT_LL_CACHE)
                                fprintf(rf, "\t%lld\t%lld",
                                        measurements->cpu_stats[measurement].cache_references,
                                        measurements->cpu_stats[measurement].cache_misses);
                            else
                                fprintf(rf, "\t---\t---");

                            if (opts->cpu_stats & CPU_STAT_BRANCHES)
                                fprintf(rf, "\t%lld\t%lld",
                                        measurements->cpu_stats[measurement].branch_instructions,
                                        measurements->cpu_stats[measurement].branch_misses);
                            else
                                fprintf(rf, "\t---\t---");
                        }
                        fprintf(rf, "\n");
                    }

                    break;
                }
                case CANNOT_SEARCH:
                {
                    fprintf(rf, "%s\t%d\t%s\t", opts->expcode, pat_len, upper_case_name);
                    write_tabbed_string(rf, "---", opts->cpu_stats ? TOTAL_COLUMNS : MEASUREMENT_COLUMNS);
                    fprintf(rf, "\n");
                    break;
                }
                case TIMED_OUT:
                {
                    fprintf(rf, "%s\t%d\t%s\t", opts->expcode, pat_len, upper_case_name);
                    write_tabbed_string(rf, "OUT", opts->cpu_stats ? TOTAL_COLUMNS : MEASUREMENT_COLUMNS);
                    fprintf(rf, "\n");
                    break;
                }
                case ERROR:
                {
                    fprintf(rf, "%s\t%d\t%s\t", opts->expcode, pat_len, upper_case_name);
                    write_tabbed_string(rf, "ERROR", opts->cpu_stats ? TOTAL_COLUMNS : MEASUREMENT_COLUMNS);
                    fprintf(rf, "\n");
                    break;
                }
            }
        }
    }

    fclose(rf);
}

void output_algorithm_statistics_csv(const smart_config_t *smart_config, const run_command_opts_t *opts, int num_pattern_lengths,
                                     benchmark_results_t *results, const algo_info_t *algorithms)
{
    FILE *rf = open_experiment_file_for_writing(smart_config, opts, "algo-statistics", "csv");

    const int MEASUREMENT_COLUMNS = 12;

    fprintf(rf, "EXPERIMENT\tPLEN\tALGORITHM");
    fprintf(rf, "\t%% Text read\tAv jump\tText bytes read\tPattern bytes read\t#Computations\t#Writes\t#Branches\t#Jumps\t#Lookups\t#Verifications");
    for (int i = 0; i < NUM_EXTRA_FIELDS; i++) {
        fprintf(rf, "\tExtra data %d", i);
    }
    fprintf(rf,"\n");

    // For each pattern length benchmarked:
    for (int pattern_len_no = 0; pattern_len_no < num_pattern_lengths; pattern_len_no++)
    {
        int pat_len = results[pattern_len_no].pattern_length;

        // For each algorithm:
        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            char upper_case_name[ALGO_NAME_LEN];
            set_upper_case_algo_name(upper_case_name, algorithms->algo_names[algo_no]);
            fprintf(rf, "%s\t%d\t%s\t", opts->expcode, pat_len, upper_case_name);

            algo_results_t *algo_res = &(results[pattern_len_no].algo_results[algo_no]);
            switch (algo_res->success_state)
            {
                case SUCCESS:
                {
                    algo_statistics_t *stats = &(algo_res->statistics);
                    fprintf(rf, "%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f",
                            opts->precision, (double) stats->sum_algo_stats.text_bytes_read / opts->num_runs /
                                                      opts->text_stats.text_actual_length * 100,
                            0, (double) (opts->text_stats.text_actual_length - pat_len) /
                               ((double) stats->sum_algo_stats.num_jumps / opts->num_runs),
                            0, (double) stats->sum_algo_stats.text_bytes_read / opts->num_runs,
                            0, (double) stats->sum_algo_stats.pattern_bytes_read / opts->num_runs,
                            0, (double) stats->sum_algo_stats.num_computations / opts->num_runs,
                            0, (double) stats->sum_algo_stats.num_writes / opts->num_runs,
                            0, (double) stats->sum_algo_stats.num_branches / opts->num_runs,
                            0, (double) stats->sum_algo_stats.num_jumps / opts->num_runs,
                            0, (double) stats->sum_algo_stats.num_lookups / opts->num_runs,
                            0, (double) stats->sum_algo_stats.num_verifications / opts->num_runs);
                    for (int i = 0; i < NUM_EXTRA_FIELDS; i++) {
                        fprintf(rf, "\t%.*f", 0, (double) stats->sum_algo_stats.extra[i] / opts->num_runs);
                    }
                    break;
                }
                case CANNOT_SEARCH:
                {
                    write_tabbed_string(rf, "---", MEASUREMENT_COLUMNS);
                    break;
                }
                case TIMED_OUT:
                {
                    write_tabbed_string(rf, "OUT", MEASUREMENT_COLUMNS);
                    break;
                }
                case ERROR:
                {
                    write_tabbed_string(rf, "ERROR", MEASUREMENT_COLUMNS);
                    break;
                }
            }
            fprintf(rf, "\n");
        }
    }

    fclose(rf);
}


/*
 * Outputs benchmark statistics as a tab separated CSV file.
 *
 * It records the minimum, maximum, mean and median timings for pre-processing, search and their total.
 * For search, it also records the standard deviation of the mean timings.
 *
 * Times are also given in Gigabytes / second, which is a more portable metric as it takes into account the length
 * of text which was searched.  If different experiments have different size texts, then only timings in Gb/s would
 * be comparable between them.
 *
 * Any CPU statistics captured will be output as additional columns on the end if they are present.
 */
void output_performance_statistics_csv(const smart_config_t *smart_config, const run_command_opts_t *opts, int num_pattern_lengths,
                                     benchmark_results_t *results, const algo_info_t *algorithms)
{
    FILE *rf = open_experiment_file_for_writing(smart_config, opts, "statistics", "csv");

    const int MEASUREMENT_COLUMNS = 18;
    const int CPU_STAT_COLUMNS = 6;
    const int TOTAL_COLUMNS = MEASUREMENT_COLUMNS + CPU_STAT_COLUMNS;

    fprintf(rf, "EXPERIMENT\tPLEN\tALGORITHM");
    fprintf(rf, "\tMIN PRE TIME (ms)\tMAX PRE TIME (ms)\tMEAN PRE TIME (ms)\tMEDIAN PRE TIME (ms)");
    fprintf(rf, "\tMIN SEARCH TIME (ms)\tMAX SEARCH TIME (ms)\tMEAN SEARCH TIME (ms)\tSTD DEVIATION\tMEDIAN SEARCH TIME (ms)");
    fprintf(rf, "\tMIN TOTAL TIME (ms)\tMAX TOTAL TIME (ms)\tMEAN TOTAL TIME (ms)\tTOTAL STD DEVIATION\tMEDIAN TOTAL TIME (ms");
    fprintf(rf, "\tMEAN SEARCH SPEED (GB/s)\tMEDIAN SEARCH SPEED (GB/s)\tMEAN TOTAL SPEED (GB/s)\tMEDIAN TOTAL SPEED (GB/s)");
    fprintf(rf, opts->cpu_stats ? "\tL1_CACHE_ACCESS\tL1_CACHE_MISSES\tLL_CACHE_ACCESS\tLL_CACHE_MISSES\tBRANCH INSTRUCTIONS\tBRANCH MISSES\n" : "\n");

    // For each pattern length benchmarked:
    for (int pattern_len_no = 0; pattern_len_no < num_pattern_lengths; pattern_len_no++)
    {
        int pat_len = results[pattern_len_no].pattern_length;

        // For each algorithm:
        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            char upper_case_name[ALGO_NAME_LEN];
            set_upper_case_algo_name(upper_case_name, algorithms->algo_names[algo_no]);
            fprintf(rf, "%s\t%d\t%s\t", opts->expcode, pat_len, upper_case_name);

            algo_results_t *algo_res = &(results[pattern_len_no].algo_results[algo_no]);
            switch (algo_res->success_state)
            {
                case SUCCESS:
                {
                    algo_statistics_t *stats = &(algo_res->statistics);
                    fprintf(rf, "%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f\t%.*f",
                            opts->precision, stats->min_pre_time,
                            opts->precision, stats->max_pre_time,
                            opts->precision, stats->mean_pre_time,
                            opts->precision, stats->median_pre_time,
                            opts->precision, stats->min_search_time,
                            opts->precision, stats->max_search_time,
                            opts->precision, stats->mean_search_time,
                            opts->precision, stats->std_search_time,
                            opts->precision, stats->median_search_time,
                            opts->precision, stats->min_total_time,
                            opts->precision, stats->max_total_time,
                            opts->precision, stats->mean_total_time,
                            opts->precision, stats->std_total_time,
                            opts->precision, stats->median_total_time);
                    fprintf(rf, "\t%.*f\t%.*f\t%.*f\t%.*f",
                            opts->precision, GBs(stats->mean_search_time, opts->text_stats.text_actual_length),
                            opts->precision, GBs(stats->median_search_time, opts->text_stats.text_actual_length),
                            opts->precision, GBs(stats->mean_pre_time + stats->mean_search_time, pat_len),
                            opts->precision, GBs(stats->median_pre_time + stats->median_search_time, pat_len));

                    if (opts->cpu_stats)
                    {
                        if (opts->cpu_stats & CPU_STAT_L1_CACHE)
                            fprintf(rf, "\t%lld\t%lld", stats->sum_cpu_stats.l1_cache_access, stats->sum_cpu_stats.l1_cache_misses);
                        else
                            fprintf(rf, "\t---\t---");

                        if (opts->cpu_stats & CPU_STAT_LL_CACHE)
                            fprintf(rf, "\t%lld\t%lld", stats->sum_cpu_stats.cache_references, stats->sum_cpu_stats.cache_misses);
                        else
                            fprintf(rf, "\t---\t---");

                        if (opts->cpu_stats & CPU_STAT_BRANCHES)
                            fprintf(rf, "\t%lld\t%lld", stats->sum_cpu_stats.branch_instructions, stats->sum_cpu_stats.branch_misses);
                        else
                            fprintf(rf, "\t---\t---");
                    }
                    break;
                }
                case CANNOT_SEARCH:
                {
                    write_tabbed_string(rf, "---", opts->cpu_stats ? TOTAL_COLUMNS : MEASUREMENT_COLUMNS);
                    break;
                }
                case TIMED_OUT:
                {
                    write_tabbed_string(rf, "OUT", opts->cpu_stats ? TOTAL_COLUMNS : MEASUREMENT_COLUMNS);
                    break;
                }
                case ERROR:
                {
                    write_tabbed_string(rf, "ERROR", opts->cpu_stats ? TOTAL_COLUMNS : MEASUREMENT_COLUMNS);
                    break;
                }
            }
            fprintf(rf, "\n");
        }
    }

    fclose(rf);
}

/*
 * Builds an array of the best times in a table for each column.
 */
void find_best_times(int rows, int cols, double table[rows][cols], double best_times[cols])
{
    // Find the best times:
    for (int col = 0; col < cols; col++)
    {
        for (int row = 0; row < rows; row++)
        {
            if (row == 0 || table[row][col] < best_times[col])
                best_times[col] = table[row][col];
        }
    }
}

/*
 * Writes a tab separated statistics table using either milliseconds or gigabytes per second.
 */
void write_text_statistics_table_as(FILE *fp, int rows, int cols, double table[rows][cols],
                                    const algo_info_t *algorithms,
                                    const run_command_opts_t *opts, benchmark_results_t *results,
                                    const char *description, enum measurement_unit unit)
{
    // Write column headers:
    fprintf(fp, "%s", "m");
    for (int patt_no = 0; patt_no < cols; patt_no++)
        fprintf(fp, "\t%d", results[patt_no].pattern_length);
    fprintf(fp, "\n");

    // Write rows:
    for (int row = 0; row < rows; row++)
    {
        char upper_case_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_name, algorithms->algo_names[row]);
        fprintf(fp, "%s", upper_case_name);
        for (int col = 0; col < cols; col++)
        {
            double value = table[row][col];
            if (value > 0)
            {
                if (unit == GIGABYTES_PER_SECOND)
                    value = GBs(value, opts->text_stats.text_actual_length);
                fprintf(fp, "\t%.*f", opts->precision, value);
            }
            else
                fprintf(fp, "\t-");
        }

        fprintf(fp, "\n");
    }

    fprintf(fp, "\nTable: %s. %s\n\n\n", description, unit_description(unit));
}

/*
 * Writes a tab separated statistics table.
 */
void write_text_statistics_table(FILE *fp, int rows, int cols, double table[rows][cols],
                                 const algo_info_t *algorithms,
                                 const run_command_opts_t *opts, benchmark_results_t *results,
                                 const char *description)
{
    write_text_statistics_table_as(fp, rows, cols, table, algorithms, opts, results, description, MILLISECONDS);
    write_text_statistics_table_as(fp, rows, cols, table, algorithms, opts, results, description, GIGABYTES_PER_SECOND);
}

/*
 * Writes a statistics table out in latex format, using the \best{#} command to highlight the best times.
 * Either writes out milliseconds or gigabytes per second as the measurement.
 */
void write_latex_statistics_table_as(FILE *fp, int rows, int cols, double table[rows][cols], const double best_times[cols],
                                    const algo_info_t *algorithms, const run_command_opts_t *opts, benchmark_results_t *results,
                                    const char *description, enum measurement_unit unit)
{
    // Write latex tabular table definition:
    fprintf(fp, "\\begin{tabular}{|l|");
    for (int col = 0; col < cols; col++)
        fprintf(fp, "%s", "l");
    fprintf(fp, "|}\n\\hline\n");

    // Write latex column headers:
    fprintf(fp, "$m$");
    for (int col = 0; col < cols; col++)
        fprintf(fp, " & $%d$", results[col].pattern_length);
    fprintf(fp, "\\\\\n");

    // Write rows:
    for (int row = 0; row < rows; row++)
    {
        char upper_case_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_name, algorithms->algo_names[row]);
        fprintf(fp, "\\textsc{%s}", upper_case_name);

        for (int col = 0; col < cols; col++) {
            if (table[row][col] < 0) {
                fprintf(fp, "%s", " & -");
            }
            else
            {
                double value = unit == MILLISECONDS ? table[row][col] : GBs(table[row][col], opts->text_stats.text_actual_length);
                if (table[row][col] <= best_times[col]) {

                    fprintf(fp, " & \\best{%.*f}", opts->precision, value);
                }
                else
                {
                    fprintf(fp, " & %.*f", opts->precision, value);
                }
            }
        }

        fprintf(fp, "\\\\\n");
    }
    fprintf(fp, "\\hline\n\\end{tabular}");
    fprintf(fp, "\n\\caption{%s. %s}\n\n\n", description, unit_description(unit));
}

/*
 * Writes a statistics table out in latex format, using the \best{#} command to highlight the best times.
 */
void write_latex_statistics_table(FILE *fp, int rows, int cols, double table[rows][cols], const double best_times[cols],
                                  const algo_info_t *algorithms, const run_command_opts_t *opts, benchmark_results_t *results,
                                  const char *description)
{
    write_latex_statistics_table_as(fp, rows, cols, table, best_times, algorithms, opts, results, description, MILLISECONDS);
    write_latex_statistics_table_as(fp, rows, cols, table, best_times, algorithms, opts, results, description, GIGABYTES_PER_SECOND);
}

/*
 * Writes a Markdown summary of a statistics table.
 */
void write_markdown_statistics_table_as(FILE *fp, int rows, int cols, double table[rows][cols], const double best_times[cols],
                                        const algo_info_t *algorithms,
                                        const run_command_opts_t *opts, benchmark_results_t *results,
                                        const char *description, enum measurement_unit unit)
{
    // Write column headers:
    fprintf(fp, "%s", "m");
    for (int patt_no = 0; patt_no < cols; patt_no++)
        fprintf(fp, " | %d", results[patt_no].pattern_length);
    fprintf(fp, "\n---");
    for (int patt_no = 0; patt_no < cols; patt_no++)
        fprintf(fp, "%s", " | ---");
    fprintf(fp, "\n");

    // Write rows:
    for (int row = 0; row < rows; row++)
    {
        char upper_case_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_name, algorithms->algo_names[row]);
        fprintf(fp, "%s", upper_case_name);
        for (int col = 0; col < cols; col++)
        {
            if (table[row][col] < 0) {
                fprintf(fp, "%s", " | -");
            } else
            {
                double value = unit == MILLISECONDS ? table[row][col] : GBs(table[row][col], opts->text_stats.text_actual_length);
                if (table[row][col] <= best_times[col]) {
                    fprintf(fp, " | **%.*f**", opts->precision, value);
                }
                else
                {
                    fprintf(fp, " | %.*f", opts->precision, value);
                }
            }
        }

        fprintf(fp, "\n");
    }

    fprintf(fp, "\nTable: %s. %s\n\n\n", description, unit_description(unit));
}

/*
 * Writes a Markdown summary of a statistics table, as both milliseconds and gigabytes per second.
 */
void write_markdown_statistics_table(FILE *fp, int rows, int cols, double table[rows][cols], const double best_times[cols],
                                     const algo_info_t *algorithms,
                                    const run_command_opts_t *opts, benchmark_results_t *results,
                                    const char *description)
{
    write_markdown_statistics_table_as(fp, rows, cols, table, best_times, algorithms, opts, results, description, MILLISECONDS);
    write_markdown_statistics_table_as(fp, rows, cols, table, best_times, algorithms, opts, results, description, GIGABYTES_PER_SECOND);
}

/*
 * Writes an HTML summary of a statistics table in either milliseconds or gigabytes per second.
 */
void write_html_statistics_table_as(FILE *fp, int rows, int cols, double table[rows][cols], const double best_times[cols],
                                    const algo_info_t *algorithms, const run_command_opts_t *opts, benchmark_results_t *results,
                                    const char *description, enum measurement_unit unit)
{
    // Write column headers:
    fprintf(fp, "%s", "<table>\n");
    fprintf(fp, "<caption style=\"caption-side:bottom\">%s. %s</caption>", description, unit_description(unit));
    fprintf(fp, "<tr><th>m</th>");
    for (int patt_no = 0; patt_no < cols; patt_no++)
        fprintf(fp, "<th>%d</th>", results[patt_no].pattern_length);
    fprintf(fp, "</tr>\n");

    // Write rows:
    for (int row = 0; row < rows; row++)
    {
        char upper_case_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_name, algorithms->algo_names[row]);
        fprintf(fp, "<tr><th>%s</th>", upper_case_name);
        for (int col = 0; col < cols; col++)
        {
            if (table[row][col] < 0) {
                fprintf(fp, "%s", "<th>-</th>");
            } else
            {
                double value = unit == MILLISECONDS ? table[row][col] : GBs(table[row][col], opts->text_stats.text_actual_length);
                if (table[row][col] <= best_times[col]) {
                    fprintf(fp, "<th><b>%.*f</b></th>", opts->precision, value);
                }
                else
                {
                    fprintf(fp, "<th>%.*f</th>", opts->precision, value);
                }
            }
        }

        fprintf(fp, "</tr>\n");
    }

    fprintf(fp, "</table>\n\n");
}

/*
 * Writes an HTML summary of a statistics table in both milliseconds and gigabytes per second.
 */
void write_html_statistics_table(FILE *fp, int rows, int cols, double table[rows][cols], const double best_times[cols],
                                 const algo_info_t *algorithms, const run_command_opts_t *opts, benchmark_results_t *results,
                                 const char *description)
{
    write_html_statistics_table_as(fp, rows, cols, table, best_times, algorithms, opts, results, description, MILLISECONDS);
    write_html_statistics_table_as(fp, rows, cols, table, best_times, algorithms, opts, results, description, GIGABYTES_PER_SECOND);
}

/*
 * Writes an XML summary of a statistics table in either milliseconds or Gigabytes per second.
 */
void write_xml_statistics_table_as(FILE *fp, int rows, int cols, double table[rows][cols], double best_times[cols],
                                   const algo_info_t *algorithms, const run_command_opts_t *opts, benchmark_results_t *results,
                                   const char *description, enum measurement_unit unit)
{

    // Write column headers:
    char data_source[MAX_LINE_LEN];
    set_data_source_description(opts, data_source);

    fprintf(fp, "<RESULTS>\n\t<CODE>%s</CODE>\n\t<TEXT>%s</TEXT>\n", opts->expcode, data_source);
    fprintf(fp, "\t<DESCRIPTION>%s. %s</DESCRIPTION>\n", description, unit_description(unit));

    // Write times for algorithms:
    for (int row = 0; row < rows; row++)
    {
        char upper_case_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_name, algorithms->algo_names[row]);
        fprintf(fp, "\t<ALGO>\n\t\t<NAME>%s</NAME>\n", upper_case_name);

        for (int col = 0; col < cols; col++)
        {
            if (table[row][col] < 0) {
                fprintf(fp, "%s", "\t\t<DATA>-</DATA>\n");
            }
            else
            {
                fprintf(fp, "\t\t<DATA><SEARCH>%.*f</SEARCH></DATA>\n", opts->precision,
                        unit == MILLISECONDS ? table[row][col] : GBs(table[row][col], opts->text_stats.text_actual_length));
            }
        }

        fprintf(fp, "\t</ALGO>\n");
    }

    // Write best times:
    fprintf(fp, "\t<BEST>\n");
    for (int col = 0; col < cols; col++)
    {
        if (best_times[col] > 0)
            fprintf(fp, "\t\t<DATA>%.*f</DATA>\n", opts->precision, best_times[col]);
        else
            fprintf(fp, "\t\t<DATA>-</DATA>\n");
    }
    fprintf(fp, "\t</BEST>\n</RESULTS>\n\n");
}

/*
 * Writes an XML summary of a statistics table.
 */
void write_xml_statistics_table(FILE *fp, int rows, int cols, double table[rows][cols], double best_times[cols],
                                 const algo_info_t *algorithms, const run_command_opts_t *opts, benchmark_results_t *results,
                                 const char *description)
{
    write_xml_statistics_table_as(fp, rows, cols, table, best_times, algorithms, opts, results, description, MILLISECONDS);
    write_xml_statistics_table_as(fp, rows, cols, table, best_times, algorithms, opts, results, description, GIGABYTES_PER_SECOND);
}

/*
 * Builds tables of statistics with the algorithms as rows and the patterns as columns,
 * for the mean and median search time, and the mean and median total times (search + preprocessing).
 */
void build_statistics_tables(int rows, int cols,
                             double mean_search[rows][cols], double median_search[rows][cols],
                             double mean_total_search[rows][cols], double median_total_search[rows][cols],
                             double best_search[rows][cols], double worst_search[rows][cols],
                             double mean_pre_time[rows][cols], double median_pre_time[rows][cols],
                             double best_pre_time[rows][cols], double worst_pre_time[rows][cols],
                             double standard_deviation[rows][cols], double standard_deviation_gbs[rows][cols],
                             const run_command_opts_t *opts, int num_pattern_lengths,
                             benchmark_results_t *results, const algo_info_t *algorithms)
{
    // For each pattern length benchmarked:
    for (int pattern_len_no = 0; pattern_len_no < num_pattern_lengths; pattern_len_no++)
    {
        // For each algorithm:
        for (int algo_no = 0; algo_no < algorithms->num_algos; algo_no++)
        {
            algo_results_t *algo_res = &(results[pattern_len_no].algo_results[algo_no]);
            switch (algo_res->success_state)
            {
                case SUCCESS:
                {
                    algo_statistics_t *stats = &(algo_res->statistics);
                    mean_search[algo_no][pattern_len_no] = stats->mean_search_time;
                    median_search[algo_no][pattern_len_no] = stats->median_search_time;
                    mean_total_search[algo_no][pattern_len_no] = stats->mean_total_time;
                    median_total_search[algo_no][pattern_len_no] = stats->median_total_time;
                    best_search[algo_no][pattern_len_no] = stats->min_search_time,
                    worst_search[algo_no][pattern_len_no] = stats->max_search_time;
                    mean_pre_time[algo_no][pattern_len_no] = stats->mean_pre_time;
                    median_pre_time[algo_no][pattern_len_no] = stats->median_pre_time;
                    best_pre_time[algo_no][pattern_len_no] = stats->min_pre_time;
                    worst_pre_time[algo_no][pattern_len_no] = stats->max_pre_time;
                    standard_deviation[algo_no][pattern_len_no] = stats->std_search_time;
                    standard_deviation_gbs[algo_no][pattern_len_no] = stats->std_search_time_gbs;
                    break;
                }
                case CANNOT_SEARCH:
                {
                    mean_search[algo_no][pattern_len_no] = -1;
                    median_search[algo_no][pattern_len_no] = -1;
                    mean_total_search[algo_no][pattern_len_no] = -1;
                    median_total_search[algo_no][pattern_len_no] = -1;
                    best_search[algo_no][pattern_len_no] = -1;
                    worst_search[algo_no][pattern_len_no] = -1;
                    mean_pre_time[algo_no][pattern_len_no] = -1;
                    median_pre_time[algo_no][pattern_len_no] = -1;
                    best_pre_time[algo_no][pattern_len_no] = -1;
                    worst_pre_time[algo_no][pattern_len_no] = -1;
                    standard_deviation[algo_no][pattern_len_no] = -1;
                    standard_deviation_gbs[algo_no][pattern_len_no] = -1;
                    break;
                }
                case TIMED_OUT:
                {
                    mean_search[algo_no][pattern_len_no] = -2;
                    median_search[algo_no][pattern_len_no] = -2;
                    mean_total_search[algo_no][pattern_len_no] = -2;
                    median_total_search[algo_no][pattern_len_no] = -2;
                    best_search[algo_no][pattern_len_no] = -2;
                    worst_search[algo_no][pattern_len_no] = -2;
                    mean_pre_time[algo_no][pattern_len_no] = -2;
                    median_pre_time[algo_no][pattern_len_no] = -2;
                    best_pre_time[algo_no][pattern_len_no] = -2;
                    worst_pre_time[algo_no][pattern_len_no] = -2;
                    standard_deviation[algo_no][pattern_len_no] = -2;
                    standard_deviation_gbs[algo_no][pattern_len_no] = -3;
                    break;
                }
                case ERROR:
                {
                    mean_search[algo_no][pattern_len_no] = -3;
                    median_search[algo_no][pattern_len_no] = -3;
                    mean_total_search[algo_no][pattern_len_no] = -3;
                    median_total_search[algo_no][pattern_len_no] = -3;
                    best_search[algo_no][pattern_len_no] = -3;
                    worst_search[algo_no][pattern_len_no] = -3;
                    mean_pre_time[algo_no][pattern_len_no] = -3;
                    median_pre_time[algo_no][pattern_len_no] = -3;
                    best_pre_time[algo_no][pattern_len_no] = -3;
                    worst_pre_time[algo_no][pattern_len_no] = -3;
                    standard_deviation[algo_no][pattern_len_no] = -3;
                    standard_deviation_gbs[algo_no][pattern_len_no] = -3;
                }
            }
        }
    }
}

/*
 * Outputs benchmark statistic tables for mean, median, best and worst statistics in a variety of text formats.
 */
void output_benchmark_statistic_tables(const smart_config_t *smart_config, const run_command_opts_t *opts,
                                       const algo_info_t *algorithms, benchmark_results_t *results, int rows, int cols,
                                       double mean_search_time_table[rows][cols], double mean_best_times[cols],
                                       double median_search_time_table[rows][cols], double median_best_times[cols],
                                       double mean_total_time_table[rows][cols], double mean_best_total_times[cols],
                                       double median_total_time_table[rows][cols], double median_best_total_times[cols],
                                       double best_time_table[rows][cols], double best_times[cols],
                                       double worst_time_table[rows][cols], double best_worst_times[cols])
{
    FILE *rf = open_experiment_file_for_writing(smart_config, opts, "text tables", "txt");

    fprintf(rf, "\nStatistics tables for experiment: %s\n", opts->expcode);
    fprintf(rf, "\nTables are provided in tab-separated CSV, LaTex, Markdown, HTML and XML formats.\n");

    fprintf(rf, "\n\nTab-separated CSV tables:\n-------------------------\n\n");
    write_text_statistics_table(rf, rows, cols, mean_search_time_table, algorithms, opts, results, "Mean search times");
    write_text_statistics_table(rf, rows, cols, median_search_time_table, algorithms, opts, results, "Median search times");
    write_text_statistics_table(rf, rows, cols, mean_total_time_table, algorithms, opts, results, "Mean total search times");
    write_text_statistics_table(rf, rows, cols, median_total_time_table, algorithms, opts, results, "Median total search times");
    write_text_statistics_table(rf, rows, cols, best_time_table, algorithms, opts, results, "Best times");
    write_text_statistics_table(rf, rows, cols, worst_time_table, algorithms, opts, results, "Worst times");

    fprintf(rf, "\n\nLatex tables:\n-------------\n\nn");
    fprintf(rf, "Place these commands in your tex file to define the format for best scores and algorithm parameters.\n");
    fprintf(rf, "\\newcommand{\\best}[1]{\\textbf{\\underline{#1}}}\n\\newcommand{\\param}[1]{$^{(#1)}$}\n\n");
    write_latex_statistics_table(rf, rows, cols, mean_search_time_table, mean_best_times, algorithms, opts, results, "Mean search times");
    write_latex_statistics_table(rf, rows, cols, median_search_time_table, median_best_times, algorithms, opts, results, "Median search times");
    write_latex_statistics_table(rf, rows, cols, mean_total_time_table, mean_best_total_times, algorithms, opts, results, "Mean total search times");
    write_latex_statistics_table(rf, rows, cols, median_total_time_table, median_best_total_times, algorithms, opts, results, "Median total search times");
    write_latex_statistics_table(rf, rows, cols, best_time_table, best_times, algorithms, opts, results, "Best times");
    write_latex_statistics_table(rf, rows, cols, worst_time_table, best_worst_times, algorithms, opts, results, "Worst times");

    fprintf(rf, "\n\nMarkdown tables:\n----------------\n\n");
    write_markdown_statistics_table(rf, rows, cols, mean_search_time_table, mean_best_times, algorithms, opts, results, "Mean search times");
    write_markdown_statistics_table(rf, rows, cols, median_search_time_table, median_best_times, algorithms, opts, results, "Median search times");
    write_markdown_statistics_table(rf, rows, cols, mean_total_time_table, mean_best_total_times, algorithms, opts, results, "Mean total search times");
    write_markdown_statistics_table(rf, rows, cols, median_total_time_table, median_best_total_times, algorithms, opts, results, "Median total search times");
    write_markdown_statistics_table(rf, rows, cols, best_time_table, best_times, algorithms, opts, results, "Best times");
    write_markdown_statistics_table(rf, rows, cols, worst_time_table, best_worst_times, algorithms, opts, results, "Worst times");

    fprintf(rf, "\n\nHTML tables:\n------------\n\n");
    write_html_statistics_table(rf, rows, cols, mean_search_time_table, mean_best_times, algorithms, opts, results, "Mean search times");
    write_html_statistics_table(rf, rows, cols, median_search_time_table, median_best_times, algorithms, opts, results, "Median search times");
    write_html_statistics_table(rf, rows, cols, mean_total_time_table, mean_best_total_times, algorithms, opts, results, "Mean total search times");
    write_html_statistics_table(rf, rows, cols, median_total_time_table, median_best_total_times, algorithms, opts, results, "Median total search times");
    write_html_statistics_table(rf, rows, cols, best_time_table, best_times, algorithms, opts, results, "Best times");
    write_html_statistics_table(rf, rows, cols, worst_time_table, best_worst_times, algorithms, opts, results, "Worst times");

    fprintf(rf, "\n\nXML tables:\n-----------\n\n");
    write_xml_statistics_table(rf, rows, cols, mean_search_time_table, mean_best_times, algorithms, opts, results, "Mean search times");
    write_xml_statistics_table(rf, rows, cols, median_search_time_table, median_best_times, algorithms, opts, results, "Median search times");
    write_xml_statistics_table(rf, rows, cols, mean_total_time_table, mean_best_total_times, algorithms, opts, results, "Mean total search times");
    write_xml_statistics_table(rf, rows, cols, median_total_time_table, median_best_total_times, algorithms, opts, results, "Median total search times");
    write_xml_statistics_table(rf, rows, cols, best_time_table, best_times, algorithms, opts, results, "Best times");
    write_xml_statistics_table(rf, rows, cols, worst_time_table, best_worst_times, algorithms, opts, results, "Worst times");

    fclose(rf);
}

/*
 * Computes a semi-random color palette using the golden ratio to produce colors which are
 * not right next to one another, making them easier to distinguish than a fully random color palette,
 * which will tend to have some colors clustered quite closely together by chance.
 */
void computeGoldenRatioColors(char colors[MAX_SELECT_ALGOS][COLOR_STRING_LENGTH])
{
    const double golden_ratio_conjugate = 0.618033988749895;
    double value = (double) rand() / RAND_MAX;
    for (int i = 0; i < MAX_SELECT_ALGOS; i++)
    {
        value += golden_ratio_conjugate;
        value = fmod(value, 1.0);
        double hue = value * 360;
        snprintf(colors[i], COLOR_STRING_LENGTH, "hsl(%.0f,50%%,50%%)", hue);
    }
}

/*
 * Writes out an interactive HTML table of results.  Best results are highlighted.
 *
 * The caption contains radio buttons to control the heat map, and checkboxes to control whether pre-processing or
 * the min-max times are also shown in the table.
 */
void write_html_table(FILE *fp, const int table_id, const char *table_description, int rows, int cols,
                      double table[rows][cols], const double best_times[cols],
                      double pre_time[rows][cols], double best_search_times[rows][cols], double worst_search_times[rows][cols],
                      const run_command_opts_t *opts, benchmark_results_t *results, const algo_info_t *algorithms)
{
    fprintf(fp, "<table id=\"resultTable%d\" class=\"exp_table\">\n", table_id);
    fprintf(fp, "<tr><td class=\"length\"></td>");
    for (int col = 0; col < cols; col++)
    {
        fprintf(fp, "<td class=\"length\">%d</td>",  results[col].pattern_length);
    }
    fprintf(fp, "</tr>");

    const char *preVisible = opts->pre ? "block" : "none";
    const char *difVisible = opts->dif ? "block" : "none";

    for (int row = 0; row < rows; row++)
    {
        fprintf(fp, "<tr>\n");
        char upper_case_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_name, algorithms->algo_names[row]);
        fprintf(fp, "<td class=\"algo\"><b>%s</b></td>\n", upper_case_name);

        for (int col = 0; col < cols; col++)
        {
            fprintf(fp, "<td><center>");

            if (pre_time[row][col] >= 0)
                fprintf(fp, "<div class=\"pre_time\" style=\"display:%s\">%.*f</div>", preVisible, opts->precision, pre_time[row][col]);
            else
                fprintf(fp, "<div class=\"pre_time\" style=\"display:%s\">-</div>", preVisible);

            if (table[row][col] < 0)
            {
                fprintf(fp, "<div class=\"search_time\">-</div>");
            }
            else if (table[row][col] <= best_times[col])
            {
//                fprintf(fp, "<div class=\"search_time_best\"><b>%.*f</b></div>", opts->precision, table[row][col]);
                fprintf(fp, "<div class=\"search_time_best\"><b> <div class=\"result_in_ms\" style=\"display:block\">%.*f</div><div class=\"result_in_gbs\" style=\"display:none\">%.*f</div>  </b></div>",
                        opts->precision, table[row][col], opts->precision,
                        GBs(table[row][col], opts->text_stats.text_actual_length));
            }
            else
            {
//                fprintf(fp, "<div class=\"search_time\">%.*f</div>",  opts->precision, table[row][col]);
                fprintf(fp, "<div class=\"search_time\"> <div class=\"result_in_ms\" style=\"display:block\">%.*f</div><div class=\"result_in_gbs\" style=\"display:none\">%.*f</div> </div>",
                        opts->precision, table[row][col], opts->precision,
                        GBs(table[row][col], opts->text_stats.text_actual_length));
            }

            if (best_search_times[row][col] >= 0 && worst_search_times[row][col] >= 0)
                fprintf(fp, "<div class=\"dif\" style=\"display:%s\">%.*f - %.*f</div>", difVisible,
                        opts->precision, best_search_times[row][col], opts->precision, worst_search_times[row][col]);
            else
                fprintf(fp, "<div class=\"dif\" style=\"display:%s\">-</div>", difVisible);

            fprintf(fp, "</center></td>");
        }
        fprintf(fp, "</tr>\n");
    }
    fprintf(fp, "</table>\n");
    fprintf(fp, "<div class=\"caption\"><b>Table %d.</b> %s running times of experimental tests n.%s. Each time value is taken from %d runs. ",
            table_id, table_description, opts->expcode, opts->num_runs);

    //TODO: do we need these descriptions given we have check boxes to control their display?
    if (opts->pre)
         fprintf(fp, "The table reports also the mean of the preprocessing time (above each time value). ");

    if (opts->dif)
         fprintf(fp, "In addition the worst and best running times are reported (below each time value). ");

    fprintf(fp, "<br><div class=\"controlHorizontalFloat\">\n"
                "<input type=\"radio\" id=\"best\" name=\"resultformat%d\" value=\"best\" checked onclick=\"setBestTableColors(document.getElementById('resultTable%d'))\">\n"
                "<label for=\"best\">Best times</label></div>\n"
                "<div class=\"controlHorizontalFloat\">\n"
                "<input type=\"radio\" id=\"heatMap5\" name=\"resultformat%d\" value=\"hm5\" onclick=\"heatMapGray(document.getElementById('resultTable%d'), 95)\">\n"
                "<label for=\"heatMap5\">Heatmap top 5%%</label></div>\n"
                "<div class=\"controlHorizontalFloat\">\n"
                "<input type=\"radio\" id=\"heatMap10\" name=\"resultformat%d\" value=\"hm10\" onclick=\"heatMapGray(document.getElementById('resultTable%d'), 90)\">\n"
                "<label for=\"heatMap25\">Heatmap top 10%%</label></div>\n"
                "<div class=\"controlHorizontalFloat\">\n"
                "<input type=\"radio\" id=\"heatMap25\" name=\"resultformat%d\" value=\"hm25\" onclick=\"heatMapGray(document.getElementById('resultTable%d'), 75)\">\n"
                "<label for=\"heatMap25\">Heatmap top 25%%</label></div>\n"
                "<div class=\"controlHorizontalFloat\">\n"
                "<input type=\"radio\" id=\"heatMap50\" name=\"resultformat%d\" value=\"hm50\" onclick=\"heatMapGray(document.getElementById('resultTable%d'), 50)\">\n"
                "<label for=\"heatMap50\">Heatmap top 50%%</label></div><div class=\"clearHorizontalFloat\"></div>\n",
                table_id, table_id, table_id, table_id, table_id, table_id, table_id, table_id, table_id, table_id);

    fprintf(fp, "<div class=\"controlHorizontalFloat\">\n");
    fprintf(fp, "<input type=\"checkbox\" id=\"pretime%d\" name=\"pretime%d\" value=\"pretime\" %s onclick=\"showChildDivs(document.getElementById('resultTable%d'), 'pre_time', this.checked)\">\n",
            table_id, table_id, (opts->pre ? "checked" : ""), table_id);
    fprintf(fp, "<label for=\"pretime%d\">Show pre-processing times</label></div>\n", table_id);

    fprintf(fp, "<div class=\"controlHorizontalFloat\">\n");
    fprintf(fp, "<input type=\"checkbox\" id=\"bestworst%d\" name=\"bestworst%d\" value=\"bestworst\" %s onclick=\"showChildDivs(document.getElementById('resultTable%d'), 'dif', this.checked)\">\n",
            table_id, table_id, (opts->dif ? "checked" : ""), table_id);
    fprintf(fp, "<label for=\"bestworst%d\">Show best and worst running times</label></div><br></div><p>\n", table_id);

    fprintf(fp, "</div><br><p>\n");
}

/*
 * Writes out an HTML header for an experiment report.
 */
void write_html_report_header(FILE *fp, const run_command_opts_t *opts)
{
    fprintf(fp, "<!DOCTYPE html><html><head>");
    fprintf(fp, "<script src=\"./js/RGraph.common.core.js\"></script>");
    fprintf(fp, "<script src=\"./js/RGraph.common.effects.js\"></script>");
    fprintf(fp, "<script src=\"./js/RGraph.line.js\"></script>");
    fprintf(fp, "<script src=\"./js/RGraph.bar.js\"></script>");
    fprintf(fp, "<script src=\"./RGraph.common.dynamic.js\"></script>");
    fprintf(fp, "<script src=\"./RGraph.common.tooltips.js\"></script>");
    fprintf(fp, "<script src=\"./js/Smart.TimeResultFormatting.js\"></script>");
    fprintf(fp, "<link href='https://fonts.googleapis.com/css?family=Dosis:300' rel='stylesheet' type='text/css'>");
    fprintf(fp, "<link href='https://fonts.googleapis.com/css?family=Yantramanav:400,100,700' rel='stylesheet' type='text/css'>");
    fprintf(fp, "<link rel=\"stylesheet\" type=\"text/css\" href=\"./js/style.css\">");
    char data_source[MAX_LINE_LEN];
    set_data_source_description(opts, data_source);
    fprintf(fp, "<title>SMART Experimental Results %s: %s</title>", opts->expcode, data_source);
    fprintf(fp, "</head>");
}

/*
 * Writes out an HTML description of the experiment.
 */
void write_html_experiment_description(FILE *fp, const run_command_opts_t *opts) {
    fprintf(fp, "<h1>SMART<span class=\"subtitle\">String Matching Algorithms Research Tool<span></h1>");
    fprintf(fp, "<h3>by Simone Faro - <span class=\"link\">www.dmi.unict.it/~faro/smart/</span> - <span class=\"link\">email: faro@dmi.unict.it</span></h3>");
    fprintf(fp, "<div class=\"name\">");
    fprintf(fp, "<h2><b>Report of Experimental Results</b></h2>");
    fprintf(fp, "<h2>Test Code %s</h2>", opts->expcode);
    char time_string[TIME_FORMAT_STRLEN];
    set_time_string_with_time(time_string, TIME_FORMAT_STRLEN, TIME_FORMAT, opts->creation_date);
    fprintf(fp, "<h2>Date %s</h2>", time_string);

    switch (opts->data_source)
    {
        case DATA_SOURCE_FILES:
        {
            char data_source[MAX_LINE_LEN];
            set_data_source_description(opts, data_source);
            fprintf(fp, "<h2>Text %s (alphabet : %d - shannon entropy : %.2f - size : %d bytes)</h2>",
                    data_source, opts->text_stats.text_alphabet, opts->text_stats.shannon_entropy_byte, opts->text_stats.text_actual_length);
            break;
        }
        case DATA_SOURCE_RANDOM:
        {
            fprintf(fp, "<h2>Text randomly generated with seed: %ld (alphabet : %d - shannon entropy : %.2f - size : %d bytes)</h2>",
                    opts->random_seed, opts->text_stats.text_alphabet, opts->text_stats.shannon_entropy_byte, opts->text_stats.text_actual_length);
            break;
        }
        case DATA_SOURCE_USER:
        {
            fprintf(fp, "<h2>Text provided on the command line (alphabet : %d - shannon entropy : %.2f - size : %d bytes)</h2>",
                    opts->text_stats.text_alphabet, opts->text_stats.shannon_entropy_byte, opts->text_stats.text_actual_length);
            break;
        }
        default: {
            fprintf(fp, "<h2>ERROR: no data source was defined.</h2>");
            break;
        }
    }
    fprintf(fp, "</div><div class=\"divClear\"/><p>");
}

/*
 * Writes a chart comparing performance of all algorithms, with different lines for each algorithm.
 */
void write_html_chart(FILE *fp, int chart_id, const char *title, int rows, int cols, double table[rows][cols],
                      const char colors [MAX_SELECT_ALGOS][COLOR_STRING_LENGTH],//const char colors[100][8],
                      const benchmark_results_t *results, const run_command_opts_t *opts, const algo_info_t *algorithms)
{
    // Chart with a key
    fprintf(fp, "<div class=\"chart_container\"><div class=\"chart_title\">%s</div>\n", title);
    fprintf(fp, "<canvas class=\"exp_chart\" id=\"cvs%d\" width=\"780\" height=\"400\">[No canvas support]</canvas>",
            chart_id);

    fprintf(fp, "<div style=\"padding-top:40px\">");

    // Draw key for chart:
    int color = 0;
    for (int row = 0; row < rows; row++)
    {
        char upper_case_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_name, algorithms->algo_names[row]);
        fprintf(fp, "<div class=\"didascalia\"><div class=\"line\" style=\"background-color:%s\"></div class=\"label\"> %s</div>\n",
                colors[color++], upper_case_name);
    }
    fprintf(fp, "</div><br/><br/>\n");

    // Caption:
    fprintf(fp, "<div class=\"caption\"><b>Chart %d.</b><p>Plot of the running times of experimental tests n.%s. ",
            chart_id, opts->expcode);
    fprintf(fp, "The x axes reports the length of the pattern while the y axes reports the running time in milliseconds. ");
    fprintf(fp, "</div>\n");
    fprintf(fp, "</div><br><p>\n");

    // Script to render the chart with the data:
    fprintf(fp,"<script>function multiChart%d(useMs) {\n", chart_id);

    // Millisecond data:
    double dymax = 0.0;
    fprintf(fp, "var dataMs = [");
    for (int row = 0; row < rows; row++)
    {
        fprintf(fp,"[");
        for (int col = 0; col < cols; col++)
        {
            if (table[row][col] < 0)
                fprintf(fp, ",");
            else
            {
                fprintf(fp, "%.*f,", opts->precision, table[row][col]);
                if (table[row][col] > dymax) dymax = table[row][col];
            }
        }
        fprintf(fp,"],\n");
    }
    fprintf(fp,"];\n");
    int ymaxMs = dymax + 1.0;

    // Gigabytes per second data:
    fprintf(fp, "var dataGbs = [");
    dymax = 0.0;
    for (int row = 0; row < rows; row++)
    {
        fprintf(fp,"[");
        for (int col = 0; col < cols; col++)
        {
            if (table[row][col] < 0)
                fprintf(fp, "0,");
            else
            {
                double gbs = GBs(table[row][col], opts->text_stats.text_actual_length);
                fprintf(fp, "%.*f,", opts->precision, gbs);
                if (gbs > dymax) dymax = gbs;
            }

        }
        fprintf(fp,"],\n");
    }
    fprintf(fp,"];\n");
    int ymaxGbs = dymax + 1.0;

    // Choose which data to use and draw lines:
    fprintf(fp, "var data = useMs ? dataMs : dataGbs;\n");
    fprintf(fp, "var ymax = useMs ? %d : %d;\n", ymaxMs, ymaxGbs);
    fprintf(fp, "var units = useMs ? \"ms\" : \"Gb/s\";");
    fprintf(fp,"var line = new RGraph.Line({\n\
            id: 'cvs%d',\n\
            data: data,\n\
            options: {\n\
            	textFont: 'Yantramanav',\n\
            	textSize: '8',\n\
            	textColor: '#444',\n\
                BackgroundBarcolor1: 'white',\n\
                BackgroundBarcolor2: 'red',\n\
                BackgroundGridColor: 'rgba(238,238,238,1)',\n\
                linewidth: 1,\n\
                filled: false,\n\
                fillstyle: ['red','blue','#0f0'],\n\
                hmargin: 5,\n\
                shadow: false,\n\
                tickmarks: 'circle',\n\
                spline: true,\n\
                gutterLeft: 40,\n\
                yaxisScaleMax: ymax,\n\
                yaxisTitle: units,\n\
                xaxisTitle: 'Pattern lengths',\n\
                xaxisLabels: [", chart_id);

    // Write x-axis pattern length labels:
    for (int col = 0; col < cols; col++)
        fprintf(fp, "'%d',",results[col].pattern_length);
    fprintf(fp,"],\n");

    // Write the line colors:
    fprintf(fp, "colors: [");
    for(int col = 0; col < color; col++) fprintf(fp, "'%s',", colors[col]);
    fprintf(fp,"],\n");

    // Draw and close the script.
    fprintf(fp,"} }).draw();\n");
    fprintf(fp,"}</script>\n");
}

/*
 * Writes an HTML javascript chart for each algorithm benchmarked.
 *
 * Each chart shows the mean performance of a single algorithm, with the minimum and maximum,
 * and the standard deviation of the search times given as shaded areas around the mean line.
 */
void write_html_algo_charts(FILE *fp, int rows, int cols,
                           double times[rows][cols], double stddev[rows][cols], double stddev_gbs[rows][cols],
                           double worst[rows][cols], double best[rows][cols],
                           const benchmark_results_t *results, const run_command_opts_t *opts, const algo_info_t *algorithms)
{
    for (int algo = 0; algo < rows; algo++) {
        char upper_case_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_name, algorithms->algo_names[algo]);

        fprintf(fp, "<div class=\"chart_container_small\"><div class=\"chart_title\">%s algorithm</div>\n",
                upper_case_name);
        fprintf(fp,
                "<div><canvas class=\"exp_chart_small\" id=\"ac%d\" width=\"460\" height=\"250\">[No canvas support]</canvas>",
                algo);
        fprintf(fp,
                "<div class=\"caption_small\">Detailed plot of the running times relative to the <b>%s algorithm</b>. ",
                upper_case_name);
        fprintf(fp, "The plot reports the mean and the distribution of the running times.");
        fprintf(fp, "</div></div></div>\n");

        fprintf(fp, "<script> function loadAlgoChart%d(useMs) {\n", algo);

        // Write out data in milliseconds:
        fprintf(fp, "var data = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else
                fprintf(fp, "%.*f,", opts->precision, times[algo][col]);
        }
        fprintf(fp, "];\n");

        // Write out data in Gigabytes per second:
        fprintf(fp, "var datagbs = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else
                fprintf(fp, "%.*f,", opts->precision, GBs(times[algo][col], opts->text_stats.text_actual_length));
        }
        fprintf(fp, "];\n");

        // Write out standard deviation lower bound in milliseconds:
        fprintf(fp, "var std1 = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else {
                // Prevent search time going below zero.
                // Subtracting the standard deviation from the mean when the data distribution is not Guassian can result
                // in a negative value.  Search algorithms data distributions are generally skewed to the right.  A lot of
                // similar fast search times but also a longer tail of slower results.  These are not purely Guassian.
                double value = MAX(0, times[algo][col] - stddev[algo][col]);
                fprintf(fp, "%.*f,", opts->precision, value);
            }
        }
        fprintf(fp, "];\n");

        //TODO: lower bound of standard is not zero?- it is the best time the algorithm achieved for that point.
        //      we can't dip below the best time achieved really, it's not informative.

        // Write out standard deviation data lower bound in gigabytes per second:
        fprintf(fp, "var std1gbs = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else {
                // Prevent search time going below zero.
                // Subtracting the standard deviation from the mean when the data distribution is not Guassian can result
                // in a negative value.  Search algorithms data distributions are generally skewed to the right.  A lot of
                // similar fast search times but also a longer tail of slower results.  These are not purely Guassian.
                double value = MAX(0, GBs(times[algo][col], opts->text_stats.text_actual_length) - stddev_gbs[algo][col]);
                fprintf(fp, "%.*f,", opts->precision, value);
            }
        }
        fprintf(fp, "];\n");

        // Write out standard deviation upper bound in milliseconds:
        fprintf(fp, "var std2 = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else
                fprintf(fp, "%.*f,", opts->precision, times[algo][col] + stddev[algo][col]);
        }
        fprintf(fp, "];\n");

        // Write out standard deviation upper bound in Gigabytes per second:
        fprintf(fp, "var std2gbs = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else
                fprintf(fp, "%.*f,", opts->precision, GBs(times[algo][col], opts->text_stats.text_actual_length) + stddev_gbs[algo][col]);
        }
        fprintf(fp, "];\n");

        // Write upper bound in ms:
        fprintf(fp, "var bound1 = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else
                fprintf(fp, "%.*f,", opts->precision, worst[algo][col]);
        }
        fprintf(fp, "];\n");

        // Write upper bound in gbs:
        fprintf(fp, "var bound1gbs = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else
                fprintf(fp, "%.*f,", opts->precision, GBs(worst[algo][col], opts->text_stats.text_actual_length));
        }
        fprintf(fp, "];\n");

        // Writer lower bound in ms.
        fprintf(fp, "var bound2 = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else
                fprintf(fp, "%.*f,", opts->precision, best[algo][col]);
        }
        fprintf(fp, "];\n");

        // Write lower bound in GB/s
        fprintf(fp, "var bound2gbs = [");
        for (int col = 0; col < cols; col++) {
            if (times[algo][col] <= 0)
                fprintf(fp, ",");
            else
                fprintf(fp, "%.*f,", opts->precision, GBs(best[algo][col], opts->text_stats.text_actual_length));
        }
        fprintf(fp, "];\n");

        double dymaxMs = 0.0, dymaxGbs = 0.0;
        for (int col = 0; col < cols; col++) {
            if (worst[algo][col] > dymaxMs)
                dymaxMs = worst[algo][col];
            if (times[algo][col] + stddev[algo][col] > dymaxMs)
                dymaxMs = times[algo][col] + stddev[algo][col];
            double gbs = GBs(best[algo][col], opts->text_stats.text_actual_length);
            if (gbs > dymaxGbs)
                dymaxGbs = gbs;
            if (gbs + stddev_gbs[algo][col] > dymaxGbs)
                dymaxGbs = gbs + stddev_gbs[algo][col];
        }
        int ymaxMs = dymaxMs + 1.0;
        int ymaxGbs = dymaxGbs + 1.0;

        fprintf(fp, "var mean_data = useMs ? data : datagbs;\n");
        fprintf(fp, "var bound_data = useMs ? [bound1, bound2] : [bound2gbs, bound1gbs];\n");
        fprintf(fp, "var std_data = useMs ? [std1, std2] : [std1gbs, std2gbs];\n");
        fprintf(fp, "var ymax = useMs ? %d : %d;\n", ymaxMs, ymaxGbs);
        fprintf(fp, "var units = useMs ? \"ms\" : \"Gb/s\";");
        fprintf(fp, "var line3 = new RGraph.Line({\n\
                id: 'ac%d',\n\
                data: bound_data,\n\
                options: {\n\
                    spline: true,\n\
                    filled: true,\n\
                    filledRange: true,\n\
                    filledColors: 'rgba(255,0,0,0.1)',\n\
                    colors: ['rgba(0,0,0,0)'],\n\
                    tickmarksStyle: null,\n\
                    yaxisScaleMax: ymax,\n\
                    yaxisScale: false,\n\
                }\n\
            }).draw();\n",
                algo);

        fprintf(fp, "var line2 = new RGraph.Line({\n\
                id: 'ac%d',\n\
                data: std_data,\n\
                options: {\n\
                    spline: true,\n\
                    filled: true,\n\
                    filledRange: true,\n\
                    filledColors: 'rgba(255,0,0,0.2)',\n\
                    colors: ['rgba(0,0,0,0)'],\n\
                    tickmarksStyle: null,\n\
                    yaxisScaleMax: ymax,\n\
                    yaxisScale: false,\n\
                }\n\
            }).draw();\n",
                algo);

        fprintf(fp, "var line = new RGraph.Line({\n\
                id: 'ac%d',\n\
                data: mean_data,\n\
                options: {\n\
                    textFont: 'Yantramanav',\n\
                    textSize: '8',\n\
                    textColor: '#444',\n\
                    BackgroundBarcolor1: 'white',\n\
                    BackgroundBarcolor2: 'red',\n\
                    BackgroundGridColor: 'rgba(238,238,238,1)',\n\
                    linewidth: 1,\n\
                    filled: false,\n\
                    hmargin: 5,\n\
                    shadow: false,\n\
                    tickmarks: 'circle',\n\
                    yaxisScaleMax: ymax,\n\
                    ylabels: false,\n\
                    spline: true,\n\
                    gutterLeft: 40,\n\
                    tickmarks: null,\n\
                    yaxisTitle: units,\n\
                    yaxisTitleOffsetx: 8,\n\
                    xaxisTitle: 'Pattern lengths',\n\
                    xaxisLabels: [",
                algo);

        for (int col = 0; col < cols; col++) {
            fprintf(fp, "'%d',", results[col].pattern_length);
        }
        fprintf(fp, "],\n");

        fprintf(fp, "colors: ['#000000'],\n");
        fprintf(fp, "} }).draw();");

        fprintf(fp, "}</script>\n");
    }
}

/*
 * Writes radio buttons to choose between displaying in milliseconds (ms) or gigabytes per second (Gb/s).
 */
void write_html_result_unit_choice(FILE *fp)
{
    fprintf(fp, "<br><div class=\"controlHorizontalFloat\">\n");
    fprintf(fp, "<input type=\"radio\" id=\"msUnit\" name=\"resultUnitType\" value=\"ms\" checked onclick=\"setPageUnits()\">\n"
                "<label for=\"msUnit\">Show results in milliseconds (ms)</label></div>\n");
    fprintf(fp, "<div class=\"controlHorizontalFloat\">\n");
    fprintf(fp, "<input type=\"radio\" id=\"gbsUnit\" name=\"resultUnitType\" value=\"gbs\" onclick=\"setPageUnits()\">\n"
                "<label for=\"gbsUnit\">Show results in gigabytes per second (Gb/s)</label></div><div class=\"clearHorizontalFloat\"></div>\n");
}

/*
 * Outputs an HTML report containing interactive tables and charts of the mean, median, best and worst results.
 */
void output_html_report(const smart_config_t *smart_config, const run_command_opts_t *opts,
                        benchmark_results_t *results, const algo_info_t *algorithms, int rows, int cols,
                        double mean_search_time_table[rows][cols], double mean_best_times[cols],
                        double median_search_time_table[rows][cols], double median_best_times[cols],
                        double mean_total_time_table[rows][cols], double mean_best_total_times[cols],
                        double median_total_time_table[rows][cols], double median_best_total_times[cols],
                        double best_time_table[rows][cols], double best_best_times[cols],
                        double worst_time_table[rows][cols], double best_worst_times[cols],
                        double mean_pre_time[rows][cols], double median_pre_time[rows][cols],
                        double best_pre_time[rows][cols], double worst_pre_time[rows][cols],
                        double standard_deviation[rows][cols], double standard_deviation_gbs[rows][cols])
{
    FILE *fp = open_experiment_file_for_writing(smart_config, opts, "report", "html");

    // Compute a reasonably nice palette of colors for the lines in the charts:
    char line_colors[MAX_SELECT_ALGOS][COLOR_STRING_LENGTH];
    computeGoldenRatioColors(line_colors); // gives nicer separation of colors than a purely random palette.

    // Write the HTML header, description of the experiment and controls for result units:
    write_html_report_header(fp, opts);
    fprintf(fp, "<body><div class=\"main_container\">");
    write_html_experiment_description(fp, opts);
    write_html_result_unit_choice(fp);

    // Write tables and charts for the mean, median, best and worst search times:
    fprintf(fp, "<br><p><h2><b>Mean search times<b></h2><p>\n");
    write_html_table(fp, 1, "Mean search", rows, cols,
                     mean_search_time_table, mean_best_times,
                     mean_pre_time, best_time_table, worst_time_table,
                     opts, results, algorithms);
    write_html_chart(fp, 1, "Mean search times", rows, cols, mean_search_time_table, line_colors, results, opts, algorithms);

    fprintf(fp, "<br><p><h2><b>Mean search and preprocessing times<b></h2><p>\n");
    write_html_table(fp, 2, "Mean search and preprocessing", rows, cols,
                     mean_total_time_table, mean_best_total_times, mean_pre_time, best_time_table, worst_time_table,
                     opts, results, algorithms);
    write_html_chart(fp, 2, "Mean search and preprocessing search times", rows, cols, mean_total_time_table, line_colors, results, opts, algorithms);

    fprintf(fp, "<br><p><h2><b>Median search times<b></h2><p>\n");
    write_html_table(fp, 3, "Median search", rows, cols,
                     median_search_time_table, median_best_times, median_pre_time, best_time_table, worst_time_table,
                     opts, results, algorithms);
    write_html_chart(fp, 3, "Median search times", rows, cols, median_search_time_table, line_colors, results, opts, algorithms);

    fprintf(fp, "<br><p><h2><b>Median search and preprocessing times<b></h2><p>\n");
    write_html_table(fp, 4, "Median search and preprocessing", rows, cols,
                     median_total_time_table, median_best_total_times, median_pre_time, best_time_table, worst_time_table,
                     opts, results, algorithms);
    write_html_chart(fp, 4, "Median search and preprocessing times", rows, cols, median_total_time_table, line_colors, results, opts, algorithms);

    fprintf(fp, "<br><p><h2><b>Best search times<b></h2><p>\n");
    write_html_table(fp, 5, "Best search", rows, cols,
                     best_time_table, best_best_times, best_pre_time, best_time_table, worst_time_table,
                     opts, results, algorithms);
    write_html_chart(fp, 5, "Best search times", rows, cols, best_time_table, line_colors, results, opts, algorithms);

    fprintf(fp, "<br><p><h2><b>Worst search times<b></h2><p>\n");
    write_html_table(fp, 6, "Worst search", rows, cols,
                     worst_time_table, best_worst_times, worst_pre_time, best_time_table, worst_time_table,
                     opts, results, algorithms);
    write_html_chart(fp, 6, "Worst search times", rows, cols, worst_time_table, line_colors, results, opts, algorithms);

    fprintf(fp, "<br><p><h2><b>Algorithm performance<b></h2><p>\n");
    write_html_algo_charts(fp, rows, cols, mean_total_time_table,
                     standard_deviation, standard_deviation_gbs, worst_time_table, best_time_table,
                           results, opts, algorithms);

    // Script to draw all charts:
    fprintf(fp,"\n<script> function drawCharts() {\n");
    fprintf(fp, "let useMilliseconds = document.getElementById('msUnit').checked;\n");

    // Redraw multi line charts:
    for (int chartNo = 1; chartNo <= 6; chartNo++)
    {
        fprintf(fp, "const canvas%d = document.getElementById('cvs%d'); ", chartNo, chartNo);
        fprintf(fp, "const context%d = canvas%d.getContext('2d'); ", chartNo, chartNo);
        fprintf(fp, "context%d.clearRect(0, 0, canvas%d.width, canvas%d.height);\n", chartNo, chartNo, chartNo);
        fprintf(fp, "multiChart%d(useMilliseconds);\n", chartNo);
    }

    // Redraw individual algo charts:
    for (int algo = 0; algo < rows; algo++)
    {
        fprintf(fp, "const cnv%d = document.getElementById('ac%d'); ", algo, algo);
        fprintf(fp, "const ctx%d = cnv%d.getContext('2d'); ", algo, algo);
        fprintf(fp, "ctx%d.clearRect(0, 0, cnv%d.width, cnv%d.height);\n", algo, algo, algo);
        fprintf(fp, "loadAlgoChart%d(useMilliseconds);\n", algo);
    }
    fprintf(fp, "}\n</script>\n");

    // Script to set the page to milliseconds or gigabytes per second:
    fprintf(fp, "<script> function setPageUnits() {\n");
    fprintf(fp, "setResultUnits(document, document.getElementById('msUnit').checked);\n");
    fprintf(fp, "drawCharts();");
    fprintf(fp, "}\n</script>\n");

    // Draw all charts when window loads:
    fprintf(fp,"\n<script>window.onload = drawCharts();</script>\n");

    // Close the HTML document and file.
    fprintf(fp, "</div><br><p></body></html>");
    fclose(fp);
}

/*
 * Outputs the results of benchmarking to various files.
 *
 * There are currently no options to control what files are output - all of them will always be written.
 *
 * It's actually quite frustrating to complete a good run and realise that you had forgotten to add
 * the option that wrote the results out in the format you needed.  These files are not very large,
 * so I think it is better to simply write out all useful information for an experiment every time.
 *
 *  1. A summary of all the run options and text statistics in a two-column tab separated format.
 *  2. A CSV file (tab separated) of all the running statistics captured during benchmarking.
 *  3. A CSV file (tab separated) of the raw measurements taken during benchmarking.
 *  4. An HTML report containing statistics tables and charts of the mean, median, best and worst times for all algorithms.
 *  5. A single text file containing the mean, median best and worst times rendered into various text formats, including:
 *     CSV (tab-separated), LaTeX, Markdown, HTML and XML.
 */
void output_results(const smart_config_t *smart_config, run_command_opts_t *opts, benchmark_results_t  *results,
                    int num_pattern_lengths, const algo_info_t *algorithms)
{
    // Output the summary of the experiment, statistics and raw measurements as tab-separated CSV files.
    output_benchmark_run_summary(smart_config, opts, algorithms);

    // If assessing algorithm statistics, just output the algo stats and measurement files.
    if (opts->statistics_type == STATS_ALGORITHM)
    {
        output_algorithm_statistics_csv(smart_config, opts, num_pattern_lengths, results, algorithms);
        output_algorithm_measurements_csv(smart_config, opts, num_pattern_lengths, results, algorithms);
    }
    else // If assessing algorithm performance, output the perf stats and measurement files as well as the other breakdowns and reports.
    {
        output_performance_statistics_csv(smart_config, opts, num_pattern_lengths, results, algorithms);
        output_performance_measurements_csv(smart_config, opts, num_pattern_lengths, results, algorithms);

        // Build statistics tables with algorithms as rows and columns as pattern lengths:
        int rows = algorithms->num_algos;
        int cols = num_pattern_lengths;
        double (*mean_search_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*median_search_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*mean_total_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*median_total_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*best_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*worst_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*mean_pre_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*median_pre_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*best_pre_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*worst_pre_time_table)[cols] = malloc(sizeof(double[rows][cols]));
        double (*standard_deviation)[cols] = malloc(sizeof(double[rows][cols]));
        double (*standard_deviation_gbs)[cols] = malloc(sizeof(double[rows][cols]));

        build_statistics_tables(rows, cols, mean_search_time_table, median_search_time_table,
                                mean_total_time_table, median_total_time_table,
                                best_time_table, worst_time_table,
                                mean_pre_time_table, median_pre_time_table,
                                best_pre_time_table, worst_pre_time_table, standard_deviation, standard_deviation_gbs,
                                opts, num_pattern_lengths, results, algorithms);

        // Get the best times for each of the tables for each pattern length.
        double best_mean_times[cols], best_median_times[cols], best_mean_total_times[cols], best_median_total_times[cols];
        double best_best_times[cols], best_worst_times[cols];
        find_best_times(rows, cols, mean_search_time_table, best_mean_times);
        find_best_times(rows, cols, median_search_time_table, best_median_times);
        find_best_times(rows, cols, mean_total_time_table, best_mean_total_times);
        find_best_times(rows, cols, median_total_time_table, best_median_total_times);
        find_best_times(rows, cols, best_time_table, best_best_times);
        find_best_times(rows, cols, worst_time_table, best_worst_times);

        // Write out the statistics tables in TSV text, LaTex, Markdown, HTML and XML formats.
        output_benchmark_statistic_tables(smart_config, opts, algorithms, results, rows, cols,
                                          mean_search_time_table, best_mean_times,
                                          median_search_time_table, best_median_times,
                                          mean_total_time_table, best_mean_total_times,
                                          median_total_time_table, best_median_total_times,
                                          best_time_table, best_best_times,
                                          worst_time_table, best_worst_times);

        // Write out an HTML report:
        output_html_report(smart_config, opts, results, algorithms, rows, cols,
                           mean_search_time_table, best_mean_times,
                           median_search_time_table, best_median_times,
                           mean_total_time_table, best_mean_total_times,
                           median_total_time_table, best_median_total_times,
                           best_time_table, best_best_times, worst_time_table, best_worst_times,
                           mean_pre_time_table, median_pre_time_table,
                           best_pre_time_table, worst_pre_time_table, standard_deviation, standard_deviation_gbs);

        free(mean_search_time_table);
        free(mean_total_time_table);
        free(median_search_time_table);
        free(median_total_time_table);
        free(best_time_table);
        free(worst_time_table);
        free(mean_pre_time_table);
        free(median_pre_time_table);
        free(best_pre_time_table);
        free(worst_pre_time_table);
        free(standard_deviation);
        free(standard_deviation_gbs);
    }
}


