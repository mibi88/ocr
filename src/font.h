/* A small toy OCR.
 *
 * This software is licensed under the BSD-3-Clause license:
 *
 * Copyright 2026 Mibi88
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef FONT_H
#define FONT_H

#if UINT_MAX >= 4294967295
typedef unsigned int font_u32_t;
typedef int font_s32_t;
#else
typedef unsigned long font_u32_t;
typedef long font_s32_t;
#endif

#if TWOS_COMPLEMENT
typedef short font_s16_t;
#else
typedef long font_s16_t;
#endif

struct point {
    font_s32_t x, y;
    unsigned char on_curve;
};

struct glyph {
    font_u32_t contour_ends;
    struct point *points;
};

struct dir {
    font_u32_t tag;
    font_u32_t checksum;
    font_u32_t offset;
    font_u32_t size;
};

struct cmap {
    unsigned int format : 16;
    unsigned int platform_id : 16;
    font_u32_t group_num;
    font_u32_t data_cur;
};

struct font {
    unsigned int table_count : 16;

    unsigned int glyph_count : 16;
    unsigned int simple_points_max : 16;

    unsigned int units_per_em : 16;

    font_s16_t long_offsets;

    font_u32_t best_map;
    struct cmap cmap;

    font_u32_t glyf_table_pos;
    font_u32_t maxp_table_pos;
    font_u32_t loca_table_pos;
    font_u32_t cmap_table_pos;
    font_u32_t htmx_table_pos;

    unsigned int added_contours : 16;
    unsigned int advance_width_count : 16;

    unsigned char flags;
    struct dir *dirs;
};

#endif
