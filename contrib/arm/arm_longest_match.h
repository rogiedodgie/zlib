/* Copyright (C) 1995-2011, 2016 Mark Adler
 * Copyright (C) 2017 ARM Holdings Inc.
 * Authors: Adenilson Cavalcanti <adenilson.cavalcanti@arm.com>
 *          Jun He <jun.he@arm.com>
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 *  claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#ifndef __ARM_LONGEST__MATCH__
#define __ARM_LONGEST__MATCH__

#if defined(ARM_NEON)
#include "deflate.h"
#include <stdint.h>
inline static long ZLIB_INTERNAL get_match_len(const unsigned char *a, const unsigned char *b, long max)
{
    int len = 0;
    unsigned long xor = 0;
    int check_loops = max/sizeof(unsigned long);
    while(check_loops-- > 0) {
        xor = (*(unsigned long *)(a+len)) ^ (*(unsigned long *)(b+len));
        if (xor) break;
        len += sizeof(unsigned long);
    }
    if (0 == xor) {
        while (len < max) {
            if (a[len] != b[len]) break;
            len++;
        }
        return len;
    }
    xor = __builtin_ctzl(xor)>>3;
    return len + xor;
}

/*
 * This implementation is based on algorithm described at:
 * http://www.gildor.org/en/projects/zlib
 * It uses the hash chain indexed by the most distant hash code to
 * reduce number of checks.
 * This also eliminates the those unnecessary check loops in legacy
 * longest_match's do..while loop if the "most distant code" is out
 * of search buffer
 *
 */
inline static unsigned ZLIB_INTERNAL arm_longest_match(deflate_state *const s, IPos cur_match) {
    unsigned chain_length = s->max_chain_length;/* max hash chain length */
    unsigned char *scan = s->window + s->strstart; /* current string */
    unsigned char *match;                       /* matched string */
    unsigned int len;                  /* length of current match */
    unsigned int best_len = s->prev_length;     /* best match length so far */
    unsigned int nice_match = s->nice_match;    /* stop if match long enough */
    IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
        s->strstart - (IPos)MAX_DIST(s) : 0;
    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */
    int offset = 0;  /* offset of the head[most_distant_hash] from IN cur_match */
    Pos *prev = s->prev;
    unsigned int wmask = s->w_mask;
    unsigned char *scan_buf_base = s->window;

    /* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
     * It is easy to get rid of this optimization if necessary.
     */
    Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");

    /* Do not look for matches beyond the end of the input. This is necessary
     * to make deflate deterministic.
     */
    if ((unsigned int)nice_match > s->lookahead) nice_match = s->lookahead;

    Assert((unsigned long)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");

    /* find most distant hash code for lazy_match */
    if (best_len > MIN_MATCH) {
        /* search for most distant hash code */
        int i;
        uint16_t hash = 0;
        IPos pos;

        UPDATE_HASH(s, hash, scan[1]);
        UPDATE_HASH(s, hash, scan[2]);
        for (i = 3; i <= best_len; i++) {
            UPDATE_HASH(s, hash, scan[i]);
            /* get head IPos of hash calced by scan[i-2..i] */
            pos = s->head[hash];
            /* compare it to current "farthest hash" IPos */
            if (pos <= cur_match) {
                /* we have a new "farthest hash" now */
                offset = i - 2;
                cur_match = pos;
            }
        }

        /* update variables to correspond offset */
        limit += offset;
        /*
         * check if the most distant code's offset is out of search buffer
         * if it is true, then this means scan[offset..offset+2] are not
	 * presented in the search buffer. So we just return best_len 
	 * we've found.
         */
        if (cur_match < limit) return best_len;

        scan_buf_base -= offset;
        /* reduce hash search depth based on best_len */
        chain_length /= best_len - MIN_MATCH;
    }

    do {
        Assert(cur_match < s->strstart, "no future");

        /* Determine matched length at current pos */
        match = scan_buf_base + cur_match;
        len = get_match_len(match, scan, MAX_MATCH);

        if (len > best_len) {
            /* found longer string */
            s->match_start = cur_match - offset;
            best_len = len;
            /* good enough? */
            if (len >= nice_match) break;
        }
        /* move to prev pos in this hash chain */
    } while ((cur_match = prev[cur_match & wmask]) > limit && --chain_length != 0);

    return (best_len <= s->lookahead)? best_len : s->lookahead;
}

#endif
#endif
