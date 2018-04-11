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
#ifndef __NEON_SLIDE_HASH__
#define __NEON_SLIDE_HASH__

#if (defined(__ARM_NEON__) || defined(__ARM_NEON))
#include "deflate.h"
#include <arm_neon.h>

inline static void ZLIB_INTERNAL neon_slide_hash(deflate_state *s)
{
    /*
     * This is ASIMD implementation for hash table rebase
     * it assumes:
     * 1. hash chain offset (Pos) is 2 bytes
     * 2. hash table size is multiple*128 bytes
     * #1 should be true as Pos is defined as "ush"
     * #2 should be true as hash_bits are greater that 7
     */
    unsigned n, m;
    unsigned short wsize = s->w_size;
    uint16x8_t v, *p;
    size_t size;

    size = s->hash_size*sizeof(s->head[0]);
    Assert((size % sizeof(uint16x8_t) * 8 == 0), "hash table size err");

    Assert(sizeof(Pos) == 2, "Wrong Pos size");

    /* slide s->head */
    v = vdupq_n_u16(wsize);
    p = (uint16x8_t *)(s->head);
    n = size / (sizeof(uint16x8_t) * 8);
    do {
        p[0] = vqsubq_u16(p[0], v);
        p[1] = vqsubq_u16(p[1], v);
        p[2] = vqsubq_u16(p[2], v);
        p[3] = vqsubq_u16(p[3], v);
        p[4] = vqsubq_u16(p[4], v);
        p[5] = vqsubq_u16(p[5], v);
        p[6] = vqsubq_u16(p[6], v);
        p[7] = vqsubq_u16(p[7], v);
        p += 8;
    } while (--n);
#ifndef FASTEST
    /* slide s->prev */
    size = wsize*sizeof(s->prev[0]);

    Assert((size % sizeof(uint16x8_t) * 8 == 0), "hash table size err");

    p = (uint16x8_t *)(s->prev);
    n = size / (sizeof(uint16x8_t) * 8);
    do {
        p[0] = vqsubq_u16(p[0], v);
        p[1] = vqsubq_u16(p[1], v);
        p[2] = vqsubq_u16(p[2], v);
        p[3] = vqsubq_u16(p[3], v);
        p[4] = vqsubq_u16(p[4], v);
        p[5] = vqsubq_u16(p[5], v);
        p[6] = vqsubq_u16(p[6], v);
        p[7] = vqsubq_u16(p[7], v);
        p += 8;
    } while (--n);
#endif
}

#endif
#endif
