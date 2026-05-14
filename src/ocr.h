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
#ifndef OCR_H
#define OCR_H

#include "image.h"
#include "font.h"

struct ocr_boundingbox {
    struct {
        font_u32_t code;
        long int x_offset;
        long int y_offset;
        long int x_scale;
        long int y_scale;
    } *text;

    size_t x1, y1;
    size_t x2, y2;

    size_t len;
    size_t max;
};

struct ocr_coverage {
    font_u32_t coverage;
    struct font_glyph *glyph;
};

struct ocr {
    struct font *font;

    struct font_renderer renderer;

    struct ocr_boundingbox *boundingboxes;

    struct ocr_coverage *coverage;

    size_t boundingbox_count;

    size_t line_treshold;
    size_t char_treshold;
    size_t min_height;

    long int x_offset_min;
    long int x_offset_max;

    long int y_offset_min;
    long int y_offset_max;

    long int x_scale_min;
    long int x_scale_max;

    long int y_scale_min;
    long int y_scale_max;

    long int scale_step;

    unsigned int high_accuracy : 1;
};

#define OCR_ERROR_X(x, l) \
    x(OE_NONE) \
    x(OE_RENDERER_INIT) \
    x(OE_MEM) \
    x(OE_UNSUPPORTED) \
    l(OE_UNKNOWN)

#define OCR_X_LIST(e) e,
#define OCR_X_LIST_END(e) e

enum {
    OCR_ERROR_X(OCR_X_LIST, OCR_X_LIST_END)
};

#ifndef OCR_IMPL
#undef OCR_ERROR_X
#undef OCR_X_LIST
#undef OCR_X_LIST_END
#endif

char *ocr_get_error_str(int error);
int ocr_init(struct ocr *ocr, struct font *font, int dpi, int max_size);
int ocr_recognise(struct ocr *ocr, struct image *image);
void ocr_free(struct ocr *ocr);

#endif
