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
 * This is an implementation of the QF (Q-gram Filtering) algorithm
 * in Branislav Durian1, Hannu Peltola, Leena Salmela and Jorma Tarhio2 	
 * Bit-Parallel Search Algorithms for Long Patterns
 * International Symposium on Experimental Algorithms (SEA 2010)
 * Q is the dimension of q-grams
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
 * extra[2]    The total number of addressable bits in the hash table.  Each entry can only store Q bits in the hash table.
 * extra[3]    The number of bits set in the hash table.
 * extra[4]    The number of times we find a full pattern match AFTER the first attempt (it tries up to Q matches each time a verification occurs).
 */

#include "include/define.h"
#include "include/main.h"
#define	Q	3
#define	S	4

#define ASIZE (1<<(Q*S))
#define AMASK (ASIZE-1)
#define BSIZE 262144	/* = 2**18 */

int search(unsigned char *x, int m, unsigned char *y, int n)
{
	int count = 0;
	int i, j, k, mq1=m-Q+1, B[ASIZE];
	unsigned int D, ch, mask=AMASK;
	if(m <= Q) return -1;
	if((WORD*8) < Q) return -1;
	if(ASIZE > BSIZE)	return -1;
	
	/* Preprocessing */
   BEGIN_PREPROCESSING
	for(i=0; i<ASIZE; i++) B[i]=0;		
	ch = 0;
	for(i = m-1; i >= 0; i--) {
		ch = ((ch << S) + x[i]) & mask;
		if(i < mq1)
			B[ch] |= (1<<((m-i) % Q));
	}
   END_PREPROCESSING

    _stats.memory_used = ASIZE * sizeof(int);
    _stats.num_lookup_entries1 = ASIZE;
    _stats.extra[1] = count_non_zero_entries_uint_table(B, ASIZE);
    _stats.extra[2] = ASIZE * Q; // only Q bits can be set per entry with QF.
    _stats.extra[3] = count_set_bits_int_table(B, ASIZE);

            /* Searching */
   BEGIN_SEARCHING
   _stats.num_writes++;
    _stats.num_branches++;
    for(i=mq1-1; i<=n-Q; i+=mq1, _stats.num_writes++, _stats.num_jumps++) {
		ch = y[i+2];
		ch = (ch<<S) + y[i+1];
		ch = (ch<<S) + y[i];
        _stats.text_bytes_read += Q;
        _stats.num_computations++;
        _stats.num_writes++;

        D = B[ch & mask];
        _stats.num_writes++;
        _stats.num_lookups++;
        _stats.num_branches++;
        if( D ) {
            _stats.extra[0]++; // num times match on main loop.
		   j = i-mq1+Q;
           _stats.num_writes++;

	      more:
		   i = i-Q;
            _stats.num_writes++;
            _stats.num_branches++;
            if(i >= j) {
				ch = y[i+2];
				ch = (ch<<S) + y[i+1];
				ch = (ch<<S) + y[i];
                _stats.text_bytes_read += Q;
                _stats.num_computations++;
                _stats.num_writes++;

				D &= B[ch & mask];
                _stats.num_writes++;
                _stats.num_lookups++;
                _stats.num_branches++;
                if(D == 0) continue;
		      else goto more;
			} 
		   else {  /* verify potential matches */
			   i = j;
                _stats.num_writes++;
                k = j-Q+1;
                _stats.num_writes++;
                _stats.num_branches++;
                if(j > n-m)  {
                    j = n-m;
                    _stats.num_writes++;
                }
                _stats.num_branches++;

                int statcount = 0; // not part of algo - used to trigger when to gather extra[4] info.

                for(  ; k <= j; k++) {
                    _stats.num_branches++;
                    statcount++;

                    if (stats_verify_pattern(k, x, m, y, n) == m) {
                        count++;
                        if (statcount == 1) _stats.extra[4]++; else _stats.extra[5]++;
                    }
                    _stats.num_branches++;
                }
		   }
	   }
       _stats.num_branches++;
    }
   END_SEARCHING
	return count;
}
