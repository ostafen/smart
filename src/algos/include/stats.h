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

#define NUM_EXTRA_FIELDS 6        // Number of extra fields available for custom stats.
#define EXTRA_FIELD_NAME_LEN 11   // Max length of extra field name (including one for NUL byte at end).

/*
 * Information to track about the operation of search algorithms.
 */
typedef struct algostats
{
    /*
     * Algorithm metadata
     */
    long memory_used;             // Number of bytes of memory allocated by the search algorithm (does not include local variables like loops, constants and temp values, does include arrays)
    long num_lookup_entries1;     // Total number of available entries in the primary lookup table, if used.
    long num_lookup_entries2;     // Total number of available entries in a secondary lookup table, if used.

    /*
     * Measurements of searching:
     */
    long text_bytes_read;         // Number of bytes read from the text during search.
    long pattern_bytes_read;      // Number of bytes read from the pattern during search.
    long num_computations;        // Number of significant computations performed (e.g. calculating a hash function).
    long num_writes;              // Number of times a value is stored
    long num_branches;            // Number of branches encountered while running.
    long num_jumps;               // Number of times the search position is advanced.
    long num_lookups;             // Number of times a lookup table is consulted.
    long num_verifications;       // Number of times a verification of the pattern is attempted (whether successful or not)

    /*
     * Algorithm specific measurements.
     */
    long extra[NUM_EXTRA_FIELDS]; // Custom fields for individual algorithms to use.

} algo_stats_t;


typedef struct algostats_metadata
{
    char extra_name[NUM_EXTRA_FIELDS][EXTRA_FIELD_NAME_LEN]; // names for extra fields.
} algostats_metadata_t;


void init_metadata(algostats_metadata_t *metadata)
{
    for (int i = 0; i < NUM_EXTRA_FIELDS; i++)
    {
        metadata->extra_name[i][0] = '\0';
    }
}

/*
 * Initialises a stats structure to have zero.
 */
void init_stats(algo_stats_t *stats)
{
    stats->memory_used = 0;
    stats->num_lookup_entries1 = 0;
    stats->num_lookup_entries2 = 0;

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
    sum->memory_used += to_add->memory_used;
    sum->num_lookup_entries1 += to_add->num_lookup_entries1;
    sum->num_lookup_entries2 += to_add->num_lookup_entries2;
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

/*
 * Divides the stats in dividend by the number in divisor.
 */
void algo_stats_divide(algo_stats_t *dividend, long divisor)
{
    if (divisor != 0)
    {
        dividend->memory_used /= divisor;
        dividend->num_lookup_entries1 /= divisor;
        dividend->num_lookup_entries2 /= divisor;
        dividend->text_bytes_read /= divisor;
        dividend->pattern_bytes_read /= divisor;
        dividend->num_computations /= divisor;
        dividend->num_writes /= divisor;
        dividend->num_branches /= divisor;
        dividend->num_jumps /= divisor;
        dividend->num_lookups /= divisor;
        dividend->num_verifications /= divisor;

        for (int i = 0; i < NUM_EXTRA_FIELDS; i++)
        {
            dividend->extra[i] /= divisor;
        }
    }
}

/*
 * Functions to sum information about tables used by search algorithms.
 */

/*
 * Counts all the bits set in an unsigned integer.
 * Algorithm is by Brian Kernighan, as described in: https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
 */
unsigned long count_set_bits_uint(unsigned int value)
{
    unsigned long count;
    for (count = 0; value; count++) {
        value &= value - 1;
    }
    return count;
}

/*
 * Counts all the bits set in an unsigned integer.
 * Algorithm is by Brian Kernighan, as described in: https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
 */
unsigned long count_set_bits_char(unsigned char value)
{
    unsigned long count;
    for (count = 0; value; count++) {
        value &= value - 1;
    }
    return count;
}

/*
 * Counts all the bits set in a table of unsigned ints, of length n.
 */
unsigned long count_set_bits_int_table(int *table, int n)
{
    unsigned long count = 0;
    for (int i = 0; i < n; i++) {
        count += count_set_bits_uint(table[i]);
    }
    return count;
}

/*
 * Counts all the bits set in a table of unsigned ints, of length n.
 */
unsigned long count_set_bits_uint_table(unsigned int *table, int n)
{
    unsigned long count = 0;
    for (int i = 0; i < n; i++) {
        count += count_set_bits_uint(table[i]);
    }
    return count;
}

/*
 * Counts all the bits set in a table of unsigned ints, of length n.
 */
unsigned long count_set_bits_char_table(unsigned char *table, int n)
{
    unsigned long count = 0;
    for (int i = 0; i < n; i++) {
        count += count_set_bits_char(table[i]);
    }
    return count;
}

/*
 * Counts all the entries in a table of chars which have a non-zero value.
 */
unsigned long count_non_zero_entries_char_table(unsigned char *table, int n)
{
    unsigned long count = 0;
    for (int i = 0; i < n; i++) if (table[i] != 0) count++;
    return count;
}

/*
 * Counts all the entries in a table of ints which have a non-zero value.
 */
unsigned long count_non_zero_entries_int_table(int *table, int n)
{
    unsigned long count = 0;
    for (int i = 0; i < n; i++) if (table[i] != 0) count++;
    return count;
}

/*
 * Counts all the entries in a table of ints which have a non-zero value.
 */
unsigned long count_non_zero_entries_uint_table(unsigned int *table, int n)
{
    unsigned long count = 0;
    for (int i = 0; i < n; i++) if (table[i] != 0) count++;
    return count;
}

/*
 * Counts all the entries in a table of ints which have entries smaller than a max_value.
 */
unsigned long count_smaller_entries_uint_table(unsigned int *table, int n, int max_value)
{
    unsigned long count = 0;
    for (int i = 0; i < n; i++) if (table[i] < max_value) count++;
    return count;
}

/*
 * Counts all the entries in a table of ints which have entries smaller than a max_value.
 */
unsigned long count_smaller_entries_int_table(int *table, int n, int max_value)
{
    unsigned long count = 0;
    for (int i = 0; i < n; i++) if (table[i] < max_value) count++;
    return count;
}

#endif //SMART_ALGOSTATS_H

