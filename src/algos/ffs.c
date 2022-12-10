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
 * This is an implementation of the Forward Fast Search algorithm
 * in D. Cantone and S. Faro. 
 * Fast-Search Algorithms: New Efficient Variants of the Boyer-Moore Pattern-Matching Algorithm. 
 * J. Autom. Lang. Comb., vol.10, n.5/6, pp.589--608, (2005).
 */

#include "include/define.h"
#include "include/main.h"

/*
 * Issues
 * ======
 *  - memory allocation failure when m > 4096, too much memory for gs[m][SIGMA] on stack.
 *
 *  Memory allocation failure
 *  -------------------------
 *  Allocating the table gx[m][SIGMA] creates 256 elements for each pattern position.  When m > 4096, this
 *  tries to allocate too much memory on the stack, and we get a seg fault.  The proper fix is to allocate
 *  the memory for this table dynamically instead of on the stack.  For now, just return -1 if m > 4096.
 */

void Forward_Suffix_Function(unsigned char *x, int m, int bm_gs[][SIGMA], int alpha) {
   int init;
   int i, j, last, suffix_len, temx[m + 1];
   init = 0;
   for(i=0;i<m;i++) for(j=init; j<init+alpha;j++) bm_gs[i][j] = m+1;  // WARN: allocates within bounds of m but sets values of m + 1.
   for(i=0; i<m; i++) temx[i]=i-1;                                    // PROBABLY OK: only allocates within bounds of m, contains [-1 .. m - 2] (assuming we don't use the -1).
   for(suffix_len=0; suffix_len<=m; suffix_len++) {                   // WARN: suffix_len <= m, instead of < m - will go to m + 1 elements.
      last = m-1;                                                     // SAFE: within m
      i = temx[last];                                                 // SAFE: only references within m, except where it is -1.
      while(i>=0) {                                                   // SAFE: Don't use the -1 element
         if( bm_gs[m-suffix_len][x[i+1]]>m-1-i )                      // WARN: will reference element m + 1 of bm_gs (at position m) when suffix_len == 0.
            if(i-suffix_len<0 || (i-suffix_len>=0 && x[i-suffix_len]!=x[m-1-suffix_len]))  // ASSUME: code does not buffer overflow on pattern buffer, but its own pre-processing tables.
               bm_gs[m-suffix_len][x[i+1]]=m-1-i;                     // WARN: will reference element m + 1 of bm_gs (at position m) when suffix_len == 0.
         if((i-suffix_len >= 0 && x[i-suffix_len]==x[last-suffix_len]) || (i-suffix_len <  0)) { // ASSUME: code does not buffer overflow on pattern buffer, but its own pre-processing tables.
            temx[last]=i;                                             // SAFE: temx only contains values from [-1 .. m - 2], i is set from temx.
            last=i;                                                   // SAFE: last, i and temx[] can only contain values from [-1 .. m - 2]
         }
         i = temx[i];                                                 // SAFE: last, i and temx[] can only contain values from [-1 .. m - 2]
      }
      if(bm_gs[m-suffix_len][x[0]] > m) bm_gs[m-suffix_len][x[0]] = m; // WARN: can access element m + 1 at position m when suffix_len == 0
      temx[last]=-1;
   }
}

//WARN: cannot allocate a large gs array here.  When m gets too big, gs[m][SIGMA} becomes very large.
//      The practical limit seems to be 4096 for m.

int search(unsigned char *x, int m, unsigned char *y, int n) {
   // any pattern greater than this cannot be searched by this implementation, as it tries to allocate too much memory to gs[][].
   // Would need to allocate the gs memory dynamically with malloc to search longer patterns.
   // This heuristic limit of 4096 may be system-dependent.
   if (m > 4096)
        return -1;

   int i, j, k, s, count, gs[m + 1][SIGMA], bc[SIGMA];
   char ch = x[m-1];

   /* Preprocessing */
   BEGIN_PREPROCESSING
   Forward_Suffix_Function(x, m, gs, SIGMA);
   for (i=0; i < SIGMA; i++) bc[i]=m;
   for (j=0; j < m; j++) bc[x[j]]=m-j-1;
   for(i=0; i<=m; i++) y[n+i]=ch;                                      //NOTE: text modification - writes one more than the pattern buffer length to the end of the text.
   y[n+m+1]='\0';                                                      //NOTE: text modification - write a null terminator at n + m + 1 to the end of the text.
   END_PREPROCESSING

   /* Searching */
   BEGIN_SEARCHING
   count = 0;
   if( !memcmp(x,y,m) ) count++; 
   s=m;
   while(s<n) {
      while(k=bc[y[s]])   s += k;
      for(j=s-1, k=m-1; k>0 && x[k-1]==y[j]; k--, j--);
      if(!k && s<n) count++;
      s += gs[k][y[s+1]];
   }
   END_SEARCHING
   return count;
}



