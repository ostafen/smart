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

#define PATTERN_SIZE_MAX 4200 // maximal length of the pattern

#define SIGMA 256 // constant alphabet size
#define NumAlgo 500
#define NumPatt 17	  // maximal number of pattern lengths
#define NumSetting 15 // number of text buffers
#define MAXTIME 999.00

/* DEFINITION OF VARIABLES */
unsigned int MINLEN = 1, MAXLEN = 4200; // min length and max length of pattern size

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "timer.h"
#include "sets.h"
#include "output.h"
#include "parser.h"
#include "select.c"
#include <libgen.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dirent.h>
#include <dlfcn.h>

#define TEXT_SIZE_DEFAULT 1048576

#define TOP_EDGE_WIDTH 60

void print_top_edge(int len)
{
	int i;
	fprintf(stdout, "\t");
	for (i = 0; i < len; i++)
		fprintf(stdout, "%c", '_');
	fprintf(stdout, "\n");
}

void print_percentage(int perc)
{
	if (perc < 10)
		printf("\b\b\b\b[%d%%]", perc);
	else if (perc < 100)
		printf("\b\b\b\b\b[%d%%]", perc);
	else
		printf("\b\b\b\b[%d%%]", perc);
	fflush(stdout);
}

int load_text(const char *filename, char *T, int max_len)
{
	printf("\tLoading the file %s\n", filename);

	FILE *input = fopen(filename, "r");
	if (input == NULL)
		return -1;

	int i = 0;
	char c;
	while (i < max_len && (c = getc(input)) != EOF)
		T[i++] = c;

	fclose(input);
	return i;
}

// TODO: list all texts to a matrix, using list_regular_files().
// create a merged text, and memorize the size of each loaded buffer, so that you can replicate it
// at the ending of the text to reach max_text_size.

int merge_all_texts(const char *path, char *T, int max_text_size)
{
	int curr_size = 0;

	DIR *dir = dir = opendir(path);
	if (dir != NULL)
	{
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			if (entry->d_type == DT_REG)
			{
				// TODO: extract function join_path();
				char full_path[STR_BUF];
				memset(full_path, 0, sizeof(char) * STR_BUF);

				strcat(full_path, path);
				strcat(full_path, "/");
				strcat(full_path, entry->d_name);

				int size = load_text(full_path, T + curr_size, max_text_size - curr_size);
				if (size < 0)
					return 0;

				curr_size += size;
			}
		}
		closedir(dir);
	}

	T[curr_size < max_text_size ? curr_size : max_text_size - 1] = '\0';

	return curr_size;
}

int gen_search_text(const char *path, unsigned char *T, int max_text_size)
{
	int size = 0;
	while (size < max_text_size)
	{
		int n = merge_all_texts(path, T + size, max_text_size - size);
		if (n < 0)
			return n;
		size += n;
	}
	return size;
}

#define MAX_ALLOC_TRIALS 10

int alloc_shared_mem(int size, void **buffer, key_t *tkey)
{
	int shmid = -1;
	for (int n = 0; shmid < 0 && n < MAX_ALLOC_TRIALS; n++)
	{
		*tkey = rand() % 1000;
		shmid = shmget(*tkey, sizeof(char) * size, IPC_CREAT | 0666);
	}

	if (shmid < 0)
		return shmid;

	if ((*buffer = shmat(shmid, NULL, 0)) == (unsigned char *)-1)
	{
		printf("\nShared memory allocation failed!\nYou need at least 12Mb of shared memory\nPlease, change your system settings and try again.\n");
		perror("shmat");
		shmctl(shmid, IPC_RMID, 0);
		exit(1);
	}
	return shmid;
}

void gen_random_patterns(unsigned char **patterns, int m, unsigned char *T, int n, int num_patterns)
{
	for (int i = 0; i < num_patterns; i++)
	{
		int j;
		int k = random() % (n - m);
		for (j = 0; j < m; j++)
			patterns[i][j] = T[k + j];

		patterns[i][j] = '\0';
	}
}

double compute_average(double *T, int n)
{
	double avg = 0.0;
	for (int i = 0; i < n; i++)
		avg += T[i];
	return avg / n;
}

double compute_std(double avg, double *T, int n)
{
	double std = 0.0;
	for (int i = 0; i < n; i++)
		std += pow(avg - T[i], 2.0);

	return sqrt(std / n);
}

#define SEARCH_FUNC_NAME "search"

int load_algos(const char algo_names[][STR_BUF], int num_algos, int (**functions)(unsigned char *, int, unsigned char *, int))
{
	for (int i = 0; i < num_algos; i++)
	{
		char algo_lib_filename[STR_BUF];
		sprintf(algo_lib_filename, "bin/algos/%s.so", str2lower(algo_names[i]));

		void *lib_handle = dlopen(algo_lib_filename, RTLD_NOW);
		int (*search)(unsigned char *, int, unsigned char *, int) = dlsym(lib_handle, SEARCH_FUNC_NAME);
		if (lib_handle == NULL || search == NULL)
		{
			printf("unable to load algorithm %s\n", algo_names[i]);
			exit(1);
		}
		functions[i] = search;
	}
	return 0;
}

void free_matrix(char **M, int n)
{
	for (int i = 0; i < n; i++)
		free(M[i]);
}

double search_time, pre_time;

int run_setting(char *filename, unsigned char *T, int n, const run_command_opts_t *opts,
				char *code, char *time_format)
{
	printf("\tExperimental tests started on %s\n", time_format);

	unsigned char *pattern_list[opts->num_runs];
	for (int i = 0; i < opts->num_runs; i++) // TODO: allocate the correct size for each pattern
		pattern_list[i] = (unsigned char *)malloc(sizeof(unsigned char) * (PATTERN_SIZE_MAX + 1));

	unsigned char c;

	/*
	char logfile[100];
	sprintf(logfile, "results/%s", code);
	mkdir(logfile, 0775);
	strcat(logfile, "/errorlog.txt");
	stream = freopen(logfile, "w", stderr); // redirect of stderr
	*/

	FILE *algo_file = fopen("selected_algos", "r");
	char algo_names[MAX_SELECT_ALGOS][STR_BUF];
	int num_running = list_algos_from_file(algo_file, algo_names);

	int (*algo_functions[MAX_SELECT_ALGOS])(unsigned char *, int, unsigned char *, int);
	load_algos(algo_names, num_running, algo_functions);

	// initializes the vector which will contain running times
	// performs experiments on a text
	double SEARCH_TIME[num_running][NumPatt], PRE_TIME[num_running][NumPatt];

	for (int i = 0; i < num_running; i++)
		for (int j = 0; j < NumPatt; j++)
			SEARCH_TIME[i][j] = PRE_TIME[i][j] = 0;

	for (int pattern_size = 0; PATT_SIZE[pattern_size] > 0; pattern_size++)
	{
		if (PATT_SIZE[pattern_size] >= opts->pattern_min_len && PATT_SIZE[pattern_size] <= opts->pattern_max_len)
		{
			int m = PATT_SIZE[pattern_size];
			gen_random_patterns(pattern_list, m, T, n, opts->num_runs);

			printf("\n");
			print_top_edge(TOP_EDGE_WIDTH);

			printf("\tExperimental results on %s: %s\n", filename, code);

			printf("\tSearching for a set of %d patterns with length %d\n", opts->num_runs, m);
			printf("\tTesting %d algorithms\n", num_running);
			printf("\n");

			int current_running = 0;
			for (int algo = 0; algo < num_running; algo++)
			{
				current_running++;

				char output_line[30];
				sprintf(output_line, "\t - [%d/%d] %s ", current_running, num_running, str2upper(algo_names[algo]));
				printf("%s", output_line);
				fflush(stdout);

				for (int i = 0; i < 35 - strlen(output_line); i++)
					printf(".");

				double TIME[opts->num_runs];

				// TODO: extract this loop into a function
				int total_occur = 0;
				for (int k = 1; k <= opts->num_runs; k++)
				{
					print_percentage((100 * k) / opts->num_runs);

					unsigned char P[m + 1];
					strcpy(P, pattern_list[k - 1]);

					search_time = pre_time = 0.0;

					int occur = algo_functions[algo](P, m, T, n);
					total_occur += occur;

					if (occur <= 0)
					{
						SEARCH_TIME[algo][pattern_size] = 0;
						PRE_TIME[algo][pattern_size] = 0;
						total_occur = occur;
						break;
					}

					if (!opts->pre)
						search_time += pre_time;

					TIME[k] = search_time;
					PRE_TIME[algo][pattern_size] += pre_time;

					if (search_time > opts->time_limit_millis)
					{
						SEARCH_TIME[algo][pattern_size] = 0;
						PRE_TIME[algo][pattern_size] = 0;
						total_occur = -2;
						break;
					}
				}

				SEARCH_TIME[algo][pattern_size] = compute_average(TIME, opts->num_runs);
				PRE_TIME[algo][pattern_size] /= (double)opts->num_runs;

				if (total_occur > 0)
				{
					double std = compute_std(SEARCH_TIME[algo][pattern_size], TIME, opts->num_runs);

					printf("\b\b\b\b\b\b\b.[OK]  ");
					if (opts->pre)
						sprintf(output_line, "\t%.2f + [%.2f ± %.2f] ms", PRE_TIME[algo][pattern_size], SEARCH_TIME[algo][pattern_size], std);
					else
						sprintf(output_line, "\t[%.2f ± %.2f] ms", SEARCH_TIME[algo][pattern_size], std);

					printf("%s", output_line);
					if (opts->occ)
					{
						if (opts->pre)
							printf("\t\tocc \%d", total_occur / opts->num_runs);
						else
							printf("\tocc \%d", total_occur / opts->num_runs);
					}
					printf("\n");
				}
				else if (total_occur == 0)
					printf("\b\b\b\b\b\b\b\b.[ERROR] \n");
				else if (total_occur == -1)
					printf("\b\b\b\b\b.[--]  \n");
				else if (total_occur == -2)
					printf("\b\b\b\b\b\b.[OUT]  \n");
			}

			printf("\n");
			print_top_edge(TOP_EDGE_WIDTH);

			// TODO: extract method: output_running_times();
		}
	}

	// free memory allocated for patterns
	free_matrix(pattern_list, opts->num_runs);

	return 0;
}

void gen_experiment_code(char *code)
{
	sprintf(code, "EXP%d", (int)time(NULL));
}

void compute_alphabet_info(int *freq, int *alphabet_size, int *max_code)
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

void compute_frequency(const char *T, int n, int *freq)
{
	int nalpha = 0;
	int maxcode = 0;
	for (int j = 0; j < SIGMA; j++)
		freq[j] = 0;

	for (int j = 0; j < n; j++)
		freq[T[j]]++;
}

void print_text_info(const char *T, int n)
{
	printf("\tText buffer of dimension %d byte\n", n);

	int freq[SIGMA];
	compute_frequency(T, n, freq);

	int alphabet_size, max_code;
	compute_alphabet_info(freq, &alphabet_size, &max_code);

	printf("\tAlphabet of %d characters.\n", alphabet_size);
	printf("\tGreater chararacter has code %d.\n", max_code);
}

void run_benchmarks(run_command_opts_t *opts, char *T)
{
	char filename_list[NumSetting][50];
	int num_buffers = split_filename(opts->filename, filename_list);

	srand(time(NULL));

	char expcode[100];
	gen_experiment_code(expcode);

	printf("\tStarting experimental tests with code %s\n", expcode);

	for (int k = 0; k < num_buffers; k++)
	{
		char fullpath[100];
		strcpy(fullpath, "data/");
		strcat(fullpath, filename_list[k]);

		printf("\n\tTry to process archive %s\n", fullpath);

		int n = gen_search_text(fullpath, T, opts->text_size);
		if (n == 0)
		{
			printf("\tunable to generate search text\n");
			return;
		}
		print_text_info(T, n);

		time_t date_timer;
		char time_format[26];
		struct tm *tm_info;
		time(&date_timer);
		tm_info = localtime(&date_timer);
		strftime(time_format, 26, "%Y:%m:%d %H:%M:%S", tm_info);

		run_setting(filename_list[k], T, n, opts, expcode, time_format);
	}

	// outputINDEX(filename_list, num_buffers, expcode);
}

int exec_run(run_command_opts_t *opts)
{
	PATT_SIZE = PATT_LARGE_SIZE; // the set of pattern legths

	srand(time(NULL));

	print_logo();

	unsigned char *T = malloc(sizeof(unsigned char) * (opts->text_size + PATTERN_SIZE_MAX));

	if (!strcmp(opts->filename, "all"))
	{
		// TODO: replace filename_list with the list of all the buffers
	}

	// TODO: add context_t type to store all shared_memory variables

	run_benchmarks(opts, T);

	free(T);

	return 0;
}

int main(int argc, const char *argv[])
{
	smart_subcommand_t subcommand;
	parse_args(argc, argv, &subcommand);

	if (!strcmp(subcommand.subcommand, "select"))
	{
		exec_select((select_command_opts_t *)subcommand.opts);
		exit(0);
	}

	if (!strcmp(subcommand.subcommand, "run"))
	{
		exec_run((run_command_opts_t *)subcommand.opts);
		exit(0);
	}
}