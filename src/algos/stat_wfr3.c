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
 *
 *  THIS IS AN IMPLEMENTATION OF:
 *	ALGORITHM Weak Factor Recognizer (WFR) Using Q-Grams
 *  appeared in: Simone Faro, Domenico Cantone and Arianna Pavone.
 *  Speeding Up String Matching by Weak Factor Recognition.
 *  Proceedings of the Pague Stringology Conference 2017: pp.42-50
 *
 * PREPROCESSING:
 *		an hash value is computed for al factors of the pattern with length in [1..16]
 *		the computed hash value is always a number in [0...256*256]
 *		if w is a factor of x, and hash(w) is its hash value, than F[hash(w)]=TRUE, otherwise F[hash(w)]=FALSE
 * SEARCHING
 *		The algorithm searches for factors of the pattern using a weak recognition method
 *		the search phase is very similar to BOM.
 *		The window is scanned right to left and for each character a new hash value of the suffix of the window is computed.
 *		Let w be the suffix we scanned. If F[hash(w)]=TRUE we continue scanning the next character of the window.
 *		Otherwise we stop scanning (w is not a factor of the pattern) and jump to the right, like in BOM.
 */

/*
 * This implementation is written to gather run-time statistics about the algorithm.
 * Performance measurements of this algorithm will not be comparable, as gathering the data takes time.
 * This should not be used for performance profiling, it is used to gather run-time algorithm statistics.
 *
 * EXTRA FIELDS
 * ============
 *
 * extra[0]    Tracks the number of times the algorithm matches the first hash it sees at the end of the window.
 * extra[1]    The number of entries that remain zero in the main hash table.
 * extra[2]    The total number of addressable bits in the hash table.
 * extra[3]    The number of bits set in the hash table.
 */

#include "include/define.h"
#include "include/main.h"
#include "include/GRAPH.h"
#define Q 3
#define HASH(j) (y[j] << 4) + (y[j - 1] << 2) + y[j - 2]

int preprocessing(unsigned char *x, int m, char *F)
{
    int i, j;
    unsigned short h;
    int fact = m < 16 ? m : 16;
    for (i = 0; i < 256 * 256; i++)
        F[i] = FALSE;
    for (i = 0; i < m; i++)
    {
        int stop = (i - fact + 1) > 0 ? (i - fact + 1) : 0;
        h = 0;
        for (j = i; j >= stop; j--)
        {
            h = h << 2;
            h += x[j];
            F[h] = TRUE;
        }
    }
}

int search(unsigned char *x, int m, unsigned char *y, int n)
{
    int i, j, p, k, count, test;
    char F[256 * 256];
    unsigned short h;
    if (m < Q)
        return -1;

    BEGIN_PREPROCESSING
    /* Preprocessing */
    int plen = m;
    if (m % Q != 0)
        m = m - (m % Q);
    int mq = m - Q + 1;
    preprocessing(x, m, F);
    for (i = 0; i < m; i++)
        y[n + i] = x[i];
    END_PREPROCESSING
    set_extra_name("match 1st", 0);
    set_extra_name("non zero", 1);
    set_extra_name("total bits", 2);
    set_extra_name("set bits", 3);

    _stats.memory_used = (256 * 256 * sizeof(char)) + m; // needs to write m bytes to the text, which counts as additional space required.
    _stats.num_lookup_entries1 = 256 * 256;

    _stats.extra[1] = count_non_zero_entries_char_table(F, 256*256);
    _stats.extra[2] = 256*256;
    _stats.extra[3] = count_set_bits_char_table(F, 256*256);

    BEGIN_SEARCHING
    /* Searching */
    count = 0;
    _stats.num_writes++; // store value.
    _stats.num_branches++;
    if (stats_verify_pattern(0, x, plen, y, n) == plen) count++;

    j = m;
    _stats.num_writes++; // store value.
    _stats.num_jumps++; // advances one.
    _stats.num_branches++;
    while (j < n)
    {
        h = HASH(j);
        _stats.num_writes++; // store value.
        _stats.num_computations++;
        _stats.text_bytes_read += Q;

        i = j - m + Q;
        _stats.num_writes++; // store value.
        _stats.num_lookups++;
        _stats.num_branches++;

        if (F[h]) {
            _stats.extra[0]++; // number of times the first hash matches.
        }

        while ((test = F[h]) && j > i + Q - 1)
        {
            j -= Q;
            _stats.num_writes++; // store value.
            h = (h << 6) + HASH(j);
            _stats.num_writes++; // store value.
            _stats.text_bytes_read += Q;
            _stats.num_computations++;
            _stats.num_lookups++;
            _stats.num_branches++;
        }
        _stats.num_branches++;
        if (j == i && test)
        {
            _stats.num_branches++;
            if (stats_verify_pattern(i - Q + 1, x, plen, y, n) == plen) count++;
        }
        j += m - Q + 1;
        _stats.num_writes++; // store value.
        _stats.num_jumps++;
        _stats.num_branches++;
    }
    END_SEARCHING
    return count;
}
