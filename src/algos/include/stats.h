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

#ifndef SMART_ALGOSTATS_H
#define SMART_ALGOSTATS_H

#define NUM_EXTRA_FIELDS 3        // Number of extra fields available for custom stats.

typedef struct algostats
{
    long text_bytes_read;         // Number of bytes read from the text during search.
    long pattern_bytes_read;      // Number of bytes read from the pattern during search.
    long num_computations;        // Number of significant computations performed (e.g. calculating a hash function).
    long num_writes;              // Number of times a value is stored
    long num_branches;            // Number of branches encountered while running.
    long num_jumps;               // Number of times the search position is advanced.
    long num_lookups;             // Number of times a lookup table is consulted.
    long num_verifications;       // Number of times a verification of the pattern is attempted (whether successful or not)
    long extra[NUM_EXTRA_FIELDS]; // Custom fields for individual algorithms to use.
} algo_stats_t;

/*
 * Initialises a stats structure to have zero.
 */
void init_stats(algo_stats_t *stats)
{
    stats->text_bytes_read = 0;
    stats->pattern_bytes_read = 0;
    stats->num_computations = 0;
    stats->num_writes = 0;
    stats->num_branches = 0;
    stats->num_jumps = 0;
    stats->num_lookups = 0;
    stats->num_verifications = 0;
    for (int i = 0; i < NUM_EXTRA_FIELDS; i++)
    {
        stats->extra[i] = 0;
    }
}

/*
 * Adds the stats in to_add to the total in sum.
 */
void algo_stats_add(algo_stats_t *sum, const algo_stats_t *to_add)
{
    sum->text_bytes_read += to_add->text_bytes_read;
    sum->pattern_bytes_read += to_add->pattern_bytes_read;
    sum->num_computations += to_add->num_computations;
    sum->num_writes += to_add->num_writes;
    sum->num_branches += to_add->num_branches;
    sum->num_jumps += to_add->num_jumps;
    sum->num_lookups += to_add->num_lookups;
    sum->num_verifications += to_add->num_verifications;
    for (int i = 0; i < NUM_EXTRA_FIELDS; i++)
    {
        sum->extra[i] += to_add->extra[i];
    }
}

#endif //SMART_ALGOSTATS_H

