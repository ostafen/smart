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
                for(  ; k <= j; k++) {
                    _stats.num_branches++;
                    if (stats_verify_pattern(k, x, m, y, n) == m) count++;
                    _stats.num_branches++;
                }
		   }
	   }
       _stats.num_branches++;
    }
   END_SEARCHING
	return count;
}
