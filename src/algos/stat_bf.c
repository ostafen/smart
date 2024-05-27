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
 * This is an implementation of the Brute Force algorithm
 */

#include "include/define.h"
#include "include/main.h"

int search(unsigned char *x, int m, unsigned char *y, int n) {
    int i, count, j;
    BEGIN_PREPROCESSING
    END_PREPROCESSING

    /* Searching */
    BEGIN_SEARCHING
    count = 0;
    _stats.num_branches++;
    for (j = 0; j <= n-m; ++j, _stats.num_jumps++, _stats.num_writes++) {
        i = stats_match_length(j, x, m, y, n);
        _stats.num_writes++;
        _stats.num_branches++;
        _stats.num_verifications++;
        if (i >= m) OUTPUT(j);
        _stats.num_branches++;
    }
    END_SEARCHING
    return count;
}

