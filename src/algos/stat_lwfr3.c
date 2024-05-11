/*
 *	ALGORITHM Weak Factor Recognizer, Linear Version, (LWFR)
 *  By Simone Faro, Domenico Cantone and Arianna Pavone
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
 *      Linearity is obtained by combaining the veification phase of the KMP algorithm
 */


#include "include/define.h"
#include "include/main.h"
#define Q 3
#define HASH(j) (y[j]<<4) + (y[j-1]<<2) + y[j-2]

void preKmp(unsigned char *x, int m, int kmpNext[]) {
    int i, j;
    i = 0;
    j = kmpNext[0] = -1;
    while (i < m) {
        while (j > -1 && x[i] != x[j])
            j = kmpNext[j];
        i++;
        j++;
        if (i<m && x[i] == x[j])
            kmpNext[i] = kmpNext[j];
        else
            kmpNext[i] = j; // i == m, requires m + 1 elements in kmpNext.
    }
}

void preprocessing(unsigned char *x, int m, char *F) {
    int i,j;
    unsigned short h;
    int fact = m<16?m:16;
    //for(i=0; i<256*256; i++) F[i] = FALSE;
    long long int *G = (long long int *) F;
    int K = 65536 / sizeof(long long int);
    for(i=0; i<K; i++) G[i] = FALSE;
    for(i=0; i<m; i++) {
        int stop = (i-fact+1)>0?(i-fact+1):0;
        h = 0;
        for(j=i; j>=stop; j--) {
            h = h<<2;
            h += x[j];
            F[h] = TRUE;
        }
    }
}


int search(unsigned char *x, int m, unsigned char *y, int n) {
    int i, j, p, b, lf, count, test, kmpNext[m + 1];
    int tp, st;
    char F[256*256];
    unsigned short h;
    if(m<Q) return -1;
    
    BEGIN_PREPROCESSING
    /* Preprocessing */
    preKmp(x, m, kmpNext);
    int plen = m;
    if(m%Q!=0) m = m-(m%Q);
    int mm1 = m-1;
    int mq = m-Q+1;
    preprocessing(x,m,F);
    for(i=0; i<m; i++) y[n+i]=x[i];
    END_PREPROCESSING

    _stats.memory_used = (256*256 * sizeof(char)) + ((m+1) * sizeof(int)) + m; // needs to write m bytes to end of text, which also counts.

    BEGIN_SEARCHING
    /* Searching */
    count = 0;
    tp = 0;
    st = 0;
    _stats.num_branches++;
    if (stats_verify_pattern(0, x, plen, y, n) == plen) count++;
    j = m;
    _stats.num_jumps++; // advances one.
    _stats.num_writes += 4; // store value.
    _stats.num_branches++;
    while (j < n) {
        h = HASH(j);
        _stats.num_writes++; // store value.
        _stats.num_computations++;
        _stats.text_bytes_read += Q;

        _stats.num_lookups++; // for the first time around the loop F[h]
        _stats.num_branches++;
        while (!F[h]) {
            j += mq;
            _stats.num_writes++; // store value.
            _stats.num_jumps++;

            h = HASH(j);
            _stats.num_writes++; // store value.
            _stats.text_bytes_read += Q;
            _stats.num_computations++;
            _stats.num_lookups++; // for the next time around the loop F[h].
            _stats.num_branches++;
        }
        lf = b = j - m + Q;
        _stats.num_writes++; // store value.
        _stats.num_branches++;
        if (b < tp) {
            b = tp - 1;  //b is the maximum between lf and tp-1
            _stats.num_writes++; // store value.
        }

        _stats.num_lookups++; // F[h] for the first loop iteration.
        _stats.num_branches++;
        while ((test = F[h]) && j > b) {
            j -= Q;
            _stats.num_writes++; // store value.

            h = (h << 6) + HASH(j);
            _stats.num_writes++; // store value.
            _stats.text_bytes_read += Q;
            _stats.num_computations++;
            _stats.num_lookups++; // F[h] for the next loop iteration.
            _stats.num_branches++;
        }

        // verification
        _stats.num_branches++;
        if (j <= b && test) {
            _stats.num_verifications++;

            lf -= Q - 1;
            _stats.num_writes++; // store value.
            _stats.num_branches++;
            if (lf > tp) {
                //restart kmp aut
                tp = lf;
                st = 0;
                _stats.num_writes += 2; // store value.
            }
            _stats.num_branches++;
            while (st >= tp - lf) {
                _stats.num_branches++;
                if (st < plen) {
                    _stats.pattern_bytes_read++;
                    _stats.text_bytes_read++;
                }

                _stats.num_branches++;
                while (st < plen && x[st] == y[tp]) {
                    st++;
                    tp++;
                    _stats.num_writes += 2; // store value.
                    _stats.num_branches++;
                    if (st < plen) {
                        _stats.pattern_bytes_read++;
                        _stats.text_bytes_read++;
                    }
                    _stats.num_branches++;
                }

                _stats.num_branches++;
                if (st == plen && lf <= n - plen) count++;
                st = kmpNext[st];
                _stats.num_writes ++; // store value.
                _stats.num_lookups++;
                _stats.num_branches++;
                if (st < 0) {
                    st++;
                    tp++;
                    _stats.num_writes += 2; // store value.
                }

                _stats.num_branches++;
            }
            j = tp + mm1 - st;
            _stats.num_writes++; // store value.
            _stats.num_jumps++;
        } else {
            j += m - Q + 1;
            _stats.num_writes++; // store value.
            _stats.num_jumps++;
        }
        _stats.num_branches++;
    }
    END_SEARCHING
    return count;
}

