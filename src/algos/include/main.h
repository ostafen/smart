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

#include "timer.h"
#include "stats.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* global variables used for computing preprocessing and searching times */
double _search_time, _pre_time; // searching time and preprocessing time
algo_stats_t _stats;
TIMER timer;

int search(unsigned char *x, int m, unsigned char *y, int n);

#define BEGIN_PREPROCESSING  \
	{                        \
		timer_start(&timer); \
	}
#define BEGIN_SEARCHING      \
	{                        \
		timer_start(&timer); \
	}
#define END_PREPROCESSING                         \
	{                                             \
		timer_stop(&timer);                       \
        _pre_time = timer_elapsed(&timer) * 1000; \
	}

#define END_SEARCHING                                \
	{                                                \
		timer_stop(&timer);                          \
        _search_time = timer_elapsed(&timer) * 1000; \
	}

/*
 * Safe pattern verification which updates the statistics for bytes read.
 * Intended to replace usages of memcmp, or other naive verification methods that compare the pattern bytes to the text,
 * while updating stats correctly.
 *
 *  Note: it will not replicate more complex pattern matching, or record any branching that takes place.
 *
 * Returns the number of characters matched at position pos.
 *
 * x is the pattern to match of length m.
 * y is the text to locate a match in of length n.
 */
int stats_verify_pattern(int pos, unsigned char *x, int m, unsigned char *y, int n)
{
    _stats.num_verifications++;
    int charpos = 0;
    while (charpos < m && pos + charpos < n) {
        _stats.pattern_bytes_read++;
        _stats.text_bytes_read++;
        if (x[charpos] != y[pos + charpos]) break;
        charpos++;
    }
    return charpos;
}

int internal_search(unsigned char *x, int m, unsigned char *y, int n,
                    double *pre_time, double *search_time, algo_stats_t *algo_stats)
{
    // Init measurement stats:
    init_stats(&_stats);
    _search_time = 0.0;
    _pre_time = 0.0;

    // Search
    int occ = search(x, m, y, n);

    // Record measurement stats.
    *search_time = _search_time;
    *pre_time = _pre_time;
    memcpy(algo_stats, &_stats, sizeof(*algo_stats));

    // Return occurrences or error value (if negative).
    return occ;
}