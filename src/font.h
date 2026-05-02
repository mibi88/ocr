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

#include <limits.h>

#if UINT_MAX >= 4294967295
typedef unsigned int font_u32_t;
typedef int font_s32_t;
#else
typedef unsigned long int font_u32_t;
typedef long int font_s32_t;
#endif

#if SHRT_MAX >= 32767 && SHRT_MIN <= -32768
typedef short font_s16_t;
#elif INT_MAX >= 32767 && INT_MIN <= -32768
typedef int font_s16_t;
#else
typedef long int font_s16_t;
#endif

struct point {
    font_s32_t x, y;
    unsigned char on_curve;
};

struct glyph {
    struct point *points;
    font_u32_t *contour_ends;

    font_u32_t offset;

    font_u32_t point_count;
    font_u32_t contour_count;

    font_u32_t advance_width;

    font_u32_t code;

    font_s32_t left_side_bearing;

    font_s32_t xmin, ymin;
    font_s32_t xmax, ymax;

    unsigned int loaded : 1;
    unsigned int waits_loading : 1;
};

struct dir {
    font_u32_t tag;
    font_u32_t checksum;
    font_u32_t offset;
    font_u32_t size;
};

struct cmap {
    font_u32_t group_num;
    font_u32_t data_cur;

    unsigned int format : 16;
    unsigned int platform_id : 16;
};

enum {
    FONT_CMAP,
    FONT_GLYF,
    FONT_HEAD,
    FONT_HHEA,
    FONT_HMTX,
    FONT_LOCA,
    FONT_MAXP,
    FONT_NAME,
    FONT_POST
};

#define FONT_REQUIRED_TABLES 9

struct font {
    struct cmap cmap;

    struct dir *dirs;

    font_u32_t best_map;

    font_u32_t table_pos[FONT_REQUIRED_TABLES];

    font_s16_t long_offsets;

    unsigned int table_count : 16;

    unsigned int glyph_count : 16;
    unsigned int simple_points_max : 16;

    unsigned int units_per_em : 16;

    unsigned int added_contours : 16;
    unsigned int advance_width_count : 16;

    unsigned int style : 16;

    unsigned int flags : 8;

    unsigned int vertical : 1;
};

#define FONT_ERROR_X(x, l) \
    x(FE_NONE) \
    x(FE_OPEN) \
    x(FE_READ) \
    x(FE_SEEK) \
    x(FE_MISSING_TABLES) \
    x(FE_CORRUPTED_MAXP) \
    x(FE_CORRUPTED_HEAD) \
    x(FE_GLYPH_INDEX) \
    x(FE_CONTOUR_COUNT) \
    x(FE_TOO_MANY_FLAGS) \
    x(FE_MEM) \
    x(FE_UNSUPPORTED) \
    l(FE_UNKNOWN)

#define FONT_X_LIST(e) e,
#define FONT_X_LIST_END(e) e

enum {
    FONT_ERROR_X(FONT_X_LIST, FONT_X_LIST_END)
};

#ifndef FONT_IMPL
#undef FONT_ERROR_X
#undef FONT_X_LIST
#undef FONT_X_LIST_END
#endif

char *font_get_error_str(int error);
int font_load(struct font *font, char *file);
void font_free(struct font *font);

#endif
