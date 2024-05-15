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
 * This is an implementation of the Horspool algorithm
 * in R. N. Horspool. Practical fast searching in strings. Softw. Pract. Exp., vol.10, n.6, pp.501--506, (1980).
 */

/*
 * An instrumented implementation of Horspool.
 * This should not be used for performance profiling, it is used to gather run-time algorithm statistics.
 *
 * EXTRA FIELDS
 * ============
 *
 * Field extra[0] tracks the number of entries in the shift table whose value is less than m.
 * This is the number of distinct characters which received a smaller value when pre-processing.
 */

#include "include/define.h"
#include "include/main.h"

void Pre_Horspool(unsigned char *P, int m, int hbc[]) {
   int i;
   for(i=0;i<SIGMA;i++)   hbc[i]=m;
   for(i=0;i<m-1;i++) hbc[P[i]]=m-i-1;
}


int search(unsigned char *P, int m, unsigned char *T, int n) {
   int i, s, count, hbc[SIGMA];
   BEGIN_PREPROCESSING
   Pre_Horspool(P, m, hbc);
   END_PREPROCESSING

   _stats.memory_used = SIGMA * sizeof(int);
   _stats.num_lookup_entries1 = SIGMA;
   _stats.extra[0] = count_smaller_entries_int_table(hbc, SIGMA, m);
    set_extra_name("#small", 0);

   /* Searching */
   BEGIN_SEARCHING
   s = 0;
   count = 0;
   _stats.num_writes += 2;
   _stats.num_branches++;
   while(s<=n-m) {
      i=0;
      _stats.num_writes++;

      _stats.num_branches++;
      while(i<m && P[i]==T[s+i]) {
          _stats.text_bytes_read++;
          _stats.pattern_bytes_read++;

          i++;
          _stats.num_writes++;
         _stats.num_branches++;
      }

       _stats.num_branches++;
       _stats.num_verifications++;
       if(i==m) count++;

      s+=hbc[T[s+m-1]];
       _stats.num_writes++;
       _stats.num_lookups++;
       _stats.text_bytes_read++;
       _stats.num_branches++;
       _stats.num_jumps++;
   }
   END_SEARCHING
   return count;
}
