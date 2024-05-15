/*
 * Copyright 2022 Matt Palmer.  All rights reserved.
 *
 * This is an implementation of the LinearHashChain algorithm, (currently unpublished) by Matt Palmer.
 * It is a factor search similar to WFR or the QF family of algorithms, but with linear performance in the worst case.
 *
 * It builds a hash table containing entries for chains of hashes.  Hashes are chained together by
 * placing a fingerprint of the *next* hash into the entry for the *current* hash.  This enables
 * a check for the second hash value to be performed without requiring a second lookup in the hash table.
 *
 * It creates Q chains of hashes from the end of the pattern back to the start.  Two techniques are used to ensure
 * linear performance. (1) During the filtering phase scanning back, it will not scan back over bytes it has already
 * matched previously. (2) During the verification phase, a linear matching algorithm (KMP) is used to identify any
 * possible matches, which will not re-verify bytes which have already been matched.
 *
 * Performance is very similar to HashChain on average, but remains linear when given worst-case and low entropy data
 * (for example, a lengthy text and patterns with an alphabet of 1, e.g. the entire text is made up of the same character.)
 *
 * This implementation is written to integrate with the SMART string search benchmarking tool,
 * by Simone Faro, Matt Palmer, Stefano Stefano Scafiti and Thierry Lecroq.
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

/*
 * Alpha - the number of bits in the hash table.
 */
#define ALPHA 11

/*
 * Number of bytes in a q-gram.
 * Chain hash functions defined below must be written to process this number of bytes.
 */
#define	Q     3

/*
 * Functions and calculated parameters.
 * Hash functions must be written to use the number of bytes defined in Q. They scan backwards from the initial position.
 */
#define S                 ((ALPHA) / (Q))                          // Bit shift for each of the chain hash byte components.
#define HASH(x, p, s)     ((((x[p] << (s)) + x[p - 1]) << (s)) + x[p - 2])  // General hash function using a bitshift for each byte added.
#define CHAIN_HASH(x, p)  HASH((x), (p), (S))                      // Hash function for chain hashes, using the S3 bitshift.
#define LINK_HASH(H)      (1U << ((H) & 0x1F))                     // Hash fingerprint, taking low 5 bits of the hash to set one of 32 bits.
#define ASIZE             (1 << (ALPHA))                           // Hash table size.
#define TABLE_MASK        ((ASIZE) - 1)                            // Mask for table is one less than the power of two size.
#define Q2                (Q + Q)                                  // 2 Qs.
#define END_FIRST_QGRAM   (Q - 1)                                  // Position of the end of the first q-gram.
#define END_SECOND_QGRAM  (Q2 - 1)                                 // Position of the end of the second q-gram.

void pre_kmp(unsigned char *x, int m, int KMP[])
{
    int j = 0;
    int t = -1;
    KMP[0] = -1;
    while (j < m) {
        while (t > -1 && x[j] != x[t]) {
            t = KMP[t];
        }
        j++; t++;
        if (j < m && x[j] == x[t]) {
            KMP[j] = KMP[t];
        }
        else {
            KMP[j] = t;
        }
    }
}


/*
 * Builds the hash table B of size ASIZE for a string x of length m.
 * Returns the 32-bit hash value of matching the entire pattern.
 */
unsigned int preprocessing(const unsigned char *x, int m, unsigned int *B) {

    // 0. Zero out the hash table.
    for (int i = 0; i < ASIZE; i++) B[i] = 0;

    // 1. Calculate all the chain hashes, ending with processing the entire pattern so H has the cumulative value.
    unsigned int H;
    int start = m < Q2 ? m - END_FIRST_QGRAM : Q;
    for (int chain_no = start; chain_no >= 1; chain_no--)
    {
        H = CHAIN_HASH(x, m - chain_no);
        for (int chain_pos = m - chain_no - Q; chain_pos >= END_FIRST_QGRAM; chain_pos -=Q)
        {
            unsigned int H_last = H;
            H = CHAIN_HASH(x, chain_pos);
            B[H_last & TABLE_MASK] |= LINK_HASH(H);
        }
    }

    // 2. Add in hashes for the first qgrams that have no preceding value.  Only set a value if there is nothing there already.
    unsigned int F;
    int stop = MIN(m, END_SECOND_QGRAM);
    for (int chain_pos = END_FIRST_QGRAM; chain_pos < stop; chain_pos++)
    {
        F = CHAIN_HASH(x, chain_pos);
        if (!B[F & TABLE_MASK]) B[F & TABLE_MASK] = LINK_HASH(~F);
    }

    return H; // Return 32-bit hash value for processing the entire pattern.
}

/*
 * Searches for a pattern x of length m in a text y of length n and reports the number of occurrences found.
 */
int search(unsigned char *x, int m, unsigned char *y, int n) {
    if (m < Q) return -1;  // have to be at least Q in length to search.
    unsigned int H, V, B[ASIZE];
    int KMP[m + 1];

    /* Preprocess;ing */
    BEGIN_PREPROCESSING
    const int MQ1 = m - Q + 1;
    preprocessing(x, m, B);
    pre_kmp(x, m, KMP);
    END_PREPROCESSING

    set_extra_name("match 1st", 0);
    set_extra_name("non zero", 1);
    set_extra_name("total bits", 2);
    set_extra_name("set bits", 3);

    _stats.memory_used = (ASIZE * sizeof(unsigned int)) + ((m+1) * sizeof(int));
    _stats.num_lookup_entries1 = ASIZE;
    _stats.num_lookup_entries2 = m + 1;
    _stats.extra[1] = count_non_zero_entries_int_table(B, ASIZE);
    _stats.extra[2] = ASIZE * sizeof(unsigned int) * 8;
    _stats.extra[3] = count_set_bits_int_table(B, ASIZE);

    /* Searching */
    BEGIN_SEARCHING
    int count = 0;
    int pos = m - 1;
    int rightmost_match_pos = 0;
    int next_verify_pos = 0;
    int pattern_pos = 0;
    _stats.num_writes += 5; // store value.

    // While within the search text:
    _stats.num_branches++;
    while (pos < n) {

        // If there is a bit set for the hash:
        H = CHAIN_HASH(y, pos);
        _stats.num_writes++; // store value.
        _stats.num_computations++; // number of significant computations like calculating a hash function.
        _stats.text_bytes_read += Q;

        V = B[H & TABLE_MASK];
        _stats.num_writes++; // store value.
        _stats.num_lookups++;

        _stats.num_branches++;
        if (V) {
            _stats.extra[0]++; // Track number of times we match on the first hash value.

            // Calculate how far back to scan and update the right most match pos.
            const int end_first_qgram_pos = pos - m + Q;
            _stats.num_writes++; // store value.
            _stats.num_branches++;
            const int scan_back_pos = MAX(end_first_qgram_pos, rightmost_match_pos) + Q; //TODO: we add Q because the first thing we do is go back Q and then hash back from that.
            _stats.num_writes++; // store value.
            rightmost_match_pos = pos;
            _stats.num_writes++; // store value.
            _stats.num_computations++; // number of significant computations like calculating a hash function.

            // Look at the chain of q-grams that precede it:
            _stats.num_branches++;
            while (pos >= scan_back_pos)
            {
                pos -= Q;
                _stats.num_writes++; // store value.

                H = CHAIN_HASH(y, pos);
                _stats.num_writes++; // store value.
                _stats.num_computations++; // number of significant computations like calculating a hash function.
                _stats.text_bytes_read += Q;

                // If we have no match for this chain q-gram, break out and go around the main loop again:
                _stats.num_computations++; // number of significant computations like calculating a hash function.
                _stats.num_branches++;
                if (!(V & LINK_HASH(H))) {
                    goto shift;
                }
                V = B[H & TABLE_MASK];
                _stats.num_writes++; // store value.
                _stats.num_lookups++;
                _stats.num_branches++;
            }

            _stats.num_verifications++;

            // Matched the chain all the way back to the start - verify the pattern :
            const int window_start_pos = end_first_qgram_pos - Q + 1;
            _stats.num_writes++; // store value.
            // Check if we need to re-start KMP if our window start is after last results.
            _stats.num_branches++;
            if (window_start_pos > next_verify_pos) {
                next_verify_pos = window_start_pos;
                _stats.num_writes++; // store value.
                pattern_pos = 0;
                _stats.num_writes++; // store value.
            }

            _stats.num_branches++;
            while (pattern_pos >= next_verify_pos - window_start_pos) {

                _stats.num_branches++;
                while (pattern_pos < m) {
                    _stats.pattern_bytes_read++;
                    _stats.text_bytes_read++;
                    _stats.num_branches++;
                    if (x[pattern_pos] != y[next_verify_pos]) break;

                    pattern_pos++;
                    next_verify_pos++;
                    _stats.num_writes+=2; // store value.
                    _stats.num_branches++;
                }

                // If we matched the whole length of the pattern (and we're still inside the text), increase match count.
                _stats.num_branches++;
                if (pattern_pos == m) count++;

                // Get the next matching pattern position.
                pattern_pos = KMP[pattern_pos];
                _stats.num_writes++; // store value.
                _stats.num_lookups++;

                _stats.num_branches++;
                if (pattern_pos < 0) {
                    pattern_pos++; // pattern_pos = 0;
                    _stats.num_writes++; // store value.

                    next_verify_pos++;
                    _stats.num_writes++; // store value.
                }
            }

            pos = next_verify_pos + m - 1 - pattern_pos;
            _stats.num_writes++; // store value.
            _stats.num_jumps++;

            continue;
        }

        // Go around the main loop looking for another hash, incrementing the pos by MQ1.
        shift:
        pos += MQ1;
        _stats.num_writes++; // store value.
        _stats.num_jumps++;
        _stats.num_branches++;
    }
    END_SEARCHING

    return count;
}
