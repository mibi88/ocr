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
#include <stdlib.h>
#include <stdio.h>

#include <limits.h>

#define OCR_IMPL

#include "ocr.h"

#define STR(s) #s,
#define STR_LAST(s) #s

#define SIZE_INC 64

#define OCR_COVERAGE_TEST 0

char *ocr_get_error_str(int error) {
    char *errors[] = {
        OCR_ERROR_X(STR, STR_LAST)
    };

    return errors[error];
}

static void generate_coverage_info(struct ocr *ocr) {
    font_u32_t i;

    for(i=0;i<ocr->font->glyph_count;i++){
        size_t y;
        font_u32_t coverage = 0;

        font_renderer_glyph(&ocr->renderer, ocr->font, ocr->font->glyphs+i,
                            ocr->renderer.max_size);

        for(y=0;y<ocr->renderer.h;y++){
            size_t x;

            for(x=0;x<ocr->renderer.w;x++){
                if((ocr->renderer
                        .b[y*ocr->renderer.row_bytes+x/4]>>(x%4*2))&1){
                    coverage++;
                }
            }
        }

        ocr->coverage[i].coverage = coverage;
        ocr->coverage[i].glyph = ocr->font->glyphs+i;
    }
}

static int coverage_cmp(const void *_a, const void *_b) {
    const struct ocr_coverage *a = _a;
    const struct ocr_coverage *b = _b;

    return a->coverage < b->coverage ? -1 : a->coverage > b->coverage;
}


int ocr_init(struct ocr *ocr, struct font *font, int dpi, int max_size) {
    ocr->coverage = malloc(font->glyph_count*sizeof(struct ocr_coverage));
    if(ocr->coverage == NULL){
        return OE_MEM;
    }
    if(font_renderer_init(&ocr->renderer, font, dpi, max_size)){
        free(ocr->coverage);
        ocr->coverage = NULL;

        return OE_RENDERER_INIT;
    }
    ocr->font = font;
    ocr->boundingboxes = NULL;

    ocr->line_treshold = 0;
    ocr->char_treshold = 0;
    ocr->min_height = 5;

    ocr->min_w_tolerance = 2;

#if !OCR_COVERAGE_TEST
    ocr->x_offset_min = -1;
    ocr->x_offset_max = +1;

    ocr->y_offset_min = -1;
    ocr->y_offset_max = +1;

    ocr->x_scale_min = 192;
    ocr->x_scale_max = 320;

    ocr->y_scale_min = 192;
    ocr->y_scale_max = 320;
#else
    ocr->x_offset_min = 0;
    ocr->x_offset_max = 0;

    ocr->y_offset_min = 0;
    ocr->y_offset_max = 0;

    ocr->x_scale_min = 256;
    ocr->x_scale_max = 256;

    ocr->y_scale_min = 256;
    ocr->y_scale_max = 256;
#endif

    ocr->scale_step = 16;

    ocr->high_accuracy = 0;

    ocr->boundingbox_count = 0;

    generate_coverage_info(ocr);

    qsort(ocr->coverage, font->glyph_count, sizeof(struct ocr_coverage),
          coverage_cmp);

#if 0
    {
        size_t i;

        for(i=0;i<font->glyph_count;i++){
            printf("%u\n", ocr->coverage[i].coverage);
        }
    }
#endif

    return OE_NONE;
}

static int find_bbs(struct ocr *ocr, struct image *image){
    /* TODO: Make something better. */

    {
        /* Find lines */

        size_t i;
        size_t y;
        unsigned char previous_was_line = 0;
        size_t line_start_y;

        for(i=0,y=0;y<image->h;y++){
            size_t x;
            size_t colored_pixels = 0;

            for(x=0;x<image->w;x++,i++){
                if((image->data[i]&IMAGE_RGBAINT(255, 255, 255, 0)) !=
                   IMAGE_RGBAINT(255, 255, 255, 0)){
                    colored_pixels++;
                }
            }

            if(colored_pixels*256/image->w > ocr->line_treshold){
                if(!previous_was_line){
                    line_start_y = y;
                    previous_was_line = 1;
                }
            }else if(previous_was_line){
                if(y-line_start_y >= ocr->min_height){
                    /* Add a new bounding box */
                    void *ptr;

                    ptr = realloc(ocr->boundingboxes,
                                  (ocr->boundingbox_count+1)*
                                  sizeof(struct ocr_boundingbox));
                    if(ptr == NULL){
                        return OE_MEM;
                    }

                    ocr->boundingboxes = ptr;

                    ocr->boundingboxes[ocr->boundingbox_count].text = NULL;
                    ocr->boundingboxes[ocr->boundingbox_count].max = 0;
                    ocr->boundingboxes[ocr->boundingbox_count].len = 0;

                    ocr->boundingboxes[ocr->boundingbox_count]
                                      .y1 = line_start_y;
                    ocr->boundingboxes[ocr->boundingbox_count].y2 = y;
                    ocr->boundingboxes[ocr->boundingbox_count].x1 = 0;
                    ocr->boundingboxes[ocr->boundingbox_count]
                                      .x2 = image->w;

                    ocr->boundingbox_count++;
                }

                previous_was_line = 0;
            }
        }
    }

    {
        /* Ajust the bounding boxes on the X axis */

        size_t i;

        for(i=0;i<ocr->boundingbox_count;i++){
            struct ocr_boundingbox *bb = ocr->boundingboxes+i;

            size_t x = bb->x1;
            size_t y;

            do{
                size_t colored_pixels = 0;
                for(y=bb->y1;y<bb->y2;y++){
                    if((image->data[y*image->w+x]&
                        IMAGE_RGBAINT(255, 255, 255, 0)) !=
                       IMAGE_RGBAINT(255, 255, 255, 0)) colored_pixels++;
                }
                if(colored_pixels*256/(bb->y2-bb->y1) <= ocr->char_treshold){
                    bb->x1++;
                }else{
                    break;
                }
            }while(++x < bb->x2);

            x = bb->x2;
            do{
                size_t colored_pixels = 0;
                for(y=bb->y1;y<bb->y2;y++){
                    if((image->data[y*image->w+x]&
                        IMAGE_RGBAINT(255, 255, 255, 0)) !=
                       IMAGE_RGBAINT(255, 255, 255, 0)) colored_pixels++;
                }
                if(colored_pixels*256/(bb->y2-bb->y1) <= ocr->char_treshold){
                    bb->x2--;
                }else{
                    break;
                }
            }while(x-- > bb->x1);
        }
    }

    return 0;
}

static int add_char(struct ocr_boundingbox *bb,
                    font_u32_t c, long int x_offset, long int y_offset,
                    long int x_scale, long int y_scale) {
    void *ptr;

    printf("Char code: %08x\n", c);

    if(bb->len >= bb->max){
        ptr = realloc(bb->text, (bb->max+SIZE_INC)*sizeof(*bb->text));

        if(ptr == NULL) return 1;
        bb->text = ptr;
        bb->max += SIZE_INC;
    }

    bb->text[bb->len].code = c;
    bb->text[bb->len].x_offset = x_offset;
    bb->text[bb->len].y_offset = y_offset;
    bb->text[bb->len].x_scale = x_scale;
    bb->text[bb->len].y_scale = y_scale;

    bb->len++;

    return OE_NONE;
}

static long int test_glyph(struct ocr *ocr, struct image *image, size_t glyph,
                           size_t x1, size_t y1, size_t y2, size_t min_w,
                           long int *x_offset_out, long int *y_offset_out,
                           long int *x_scale_out, long int *y_scale_out,
                           long int *advance_out,
                           unsigned long int *not_matching_out) {
    long int sy;

    long int best_score = 0;
    unsigned long int least_not_matching = ULONG_MAX;

    long int best_x_offset = 0;
    long int best_y_offset = 0;
    long int best_x_scale = 256;
    long int best_y_scale = 256;

    long int best_advance = 1;

    font_renderer_glyph(&ocr->renderer, ocr->font,
                        ocr->font->glyphs+glyph,
                        ocr->renderer.max_size);

    if(ocr->renderer.glyph_width == 0 || ocr->renderer.glyph_height == 0){
        *x_offset_out = best_x_offset;
        *y_offset_out = best_y_offset;

        *x_scale_out = best_x_scale;
        *y_scale_out = best_y_scale;

        *not_matching_out = least_not_matching;

        return least_not_matching;
    }

    /* NOTE: This is very costly */
    for(sy=ocr->y_offset_min;sy<=ocr->y_offset_max;sy++){
        long int sx;

        for(sx=ocr->x_offset_min;sx<=ocr->x_offset_max;sx++){
            long int y_scale;

            for(y_scale=ocr->y_scale_min;
                y_scale<=ocr->y_scale_max;
                y_scale+=ocr->scale_step){
                long int x_scale;

                for(x_scale=ocr->x_scale_min;
                    x_scale<=ocr->x_scale_max;
                    x_scale+=ocr->scale_step){
                    long int y;

                    long int test_x = x1+sx*x_scale/256;
                    long int test_y = y1+sy*y_scale/256;

                    long int score = 0;

                    long int advance = ocr->renderer.advance_width*x_scale*
                                       (y2-y1)/
                                       (256*ocr->renderer.glyph_height);

                    unsigned long int not_matching = 0;

                    long int w = ocr->renderer.glyph_width*x_scale*(y2-y1)/
                                 (256*ocr->renderer.glyph_height);
                    long int h = y_scale*(y2-y1)/256;

                    if(w < (long int)min_w || !h) continue;

                    for(y=0;y<h;y++){
                        long int x;

                        for(x=0;x<w;x++){
                            unsigned char glyph_color = 0;
                            unsigned char image_color = 0;

                            long int gx = (x*256*ocr->renderer.glyph_height)/
                                          (x_scale*(y2-y1));
                            long int gy = (y*256*ocr->renderer.glyph_height)/
                                          (y_scale*(y2-y1));

                            long int ix = test_x+x;
                            long int iy = test_y+y;

                            glyph_color = (ocr->renderer
                                            .b[gy*ocr->renderer.row_bytes+gx/4]
                                                >>(gx%4*2))&1;
#if OCR_COVERAGE_TEST
                            printf("%ld, %ld, %ld, %ld\n", y2-y1, test_y,
                                   x_scale, x);
                            printf("Set pixel %ld, %ld\n", ix, iy);
#endif

                            if(ix >= 0 && ix < (long int)image->w &&
                               iy >= 0 && iy < (long int)image->h){
#if OCR_COVERAGE_TEST
                                if(glyph_color){
                                    image->data[iy*image->w+ix] =
                                                IMAGE_RGBAINT(0, 255, 0, 0);
                                }
#endif
                                image_color = (image->data[iy*image->w+ix]&
                                               IMAGE_RGBAINT(255, 255, 255, 0))
                                              != 0;
                            }

                            score += (glyph_color&image_color)-
                                     (image_color&glyph_color);
                            not_matching += glyph_color != image_color;
                        }
                    }

                    if(not_matching < least_not_matching){
                        least_not_matching = not_matching;
                        best_score = score;
                        best_x_offset = sx;
                        best_y_offset = sy;
                        best_x_scale = x_scale;
                        best_y_scale = y_scale;
                        best_advance = advance;
                    }
                }
            }
        }
    }

    *x_offset_out = best_x_offset;
    *y_offset_out = best_y_offset;

    *x_scale_out = best_x_scale;
    *y_scale_out = best_y_scale;

    *advance_out = best_advance;

    *not_matching_out = least_not_matching;

    return best_score;
}

static int process_line(struct ocr *ocr, struct ocr_boundingbox *bb,
                        struct image *image) {
    size_t x;

#if !OCR_COVERAGE_TEST
    for(x=bb->x1;x<bb->x2;){
#else
    x=bb->x1;
    {
#endif
        size_t min_w;

        unsigned long int least_not_matching = ULONG_MAX;

        long best_x_offset;
        long best_y_offset;
        long best_x_scale;
        long best_y_scale;

        long best_advance;

        long dx;

        font_u32_t glyph;

        font_u32_t i;

        int rc;

        printf("X: %lu\n", x);

        do{
            size_t colored_pixels = 0;
            size_t y;

            for(y=bb->y1;y<bb->y2;y++){
                if((image->data[y*image->w+x]&
                    IMAGE_RGBAINT(255, 255, 255, 0)) !=
                   IMAGE_RGBAINT(255, 255, 255, 0)) colored_pixels++;
            }
            if(colored_pixels*256/(bb->y2-bb->y1) <= ocr->char_treshold){
                x++;
            }else{
                break;
            }
        }while(1);

        min_w = x;

        do{
            size_t colored_pixels = 0;
            size_t y;

            for(y=bb->y1;y<bb->y2;y++){
                if((image->data[y*image->w+min_w]&
                    IMAGE_RGBAINT(255, 255, 255, 0)) !=
                   IMAGE_RGBAINT(255, 255, 255, 0)) colored_pixels++;
            }
            if(colored_pixels*256/(bb->y2-bb->y1) > ocr->char_treshold){
                min_w++;
            }else{
                break;
            }
        }while(1);

        min_w -= x;

        if(min_w > ocr->min_w_tolerance) min_w -= ocr->min_w_tolerance;

        /* TODO: Make a faster but less accurate version by performing a
         * slightly modified version of binary search using the ocr->coverage
         * array. */
#if !OCR_COVERAGE_TEST
        for(i=0;i<ocr->font->glyph_count;i++){
#else
        i = font_lookup_char(ocr->font, 'a')-ocr->font->glyphs;
        {
#endif
            unsigned long int not_matching;

            long int score;

            long int x_offset;
            long int y_offset;
            long int x_scale;
            long int y_scale;

            long int advance;

            (void)score;

#if OCR_DEBUG
            printf("Testing glyph %u/%u -- char code: %08x\n", i,
                   ocr->font->glyph_count, ocr->font->glyphs[i].code);
#endif

            score = test_glyph(ocr, image, i, x, bb->y1, bb->y2, min_w,
                               &x_offset, &y_offset, &x_scale, &y_scale,
                               &advance, &not_matching);

#if OCR_DEBUG
            printf("Not matching: %lu\n", not_matching);
#endif

            if(not_matching == 0){
                if((rc = add_char(bb, ocr->font->glyphs[i].code,
                                  x_offset, y_offset, x_scale, y_scale))){
                    return rc;
                }

                dx = advance+x_offset*x_scale/256;
                if(!dx) dx = 1;

                goto CONTINUE;
            }else if(not_matching < least_not_matching){
                least_not_matching = not_matching;
                glyph = i;

                best_x_offset = x_offset;
                best_y_offset = y_offset;
                best_x_scale = x_scale;
                best_y_scale = y_scale;

                best_advance = advance;
            }
        }

        if((rc = add_char(bb, ocr->font->glyphs[glyph].code, best_x_offset,
                          best_y_offset, best_x_scale, best_y_scale))){
            return rc;
        }

        printf("%ld, %ld\n", best_advance, best_x_offset*best_x_scale/256);

        dx = best_advance+best_x_offset*best_x_scale/256;
        if(!dx) dx = 1;

CONTINUE:
        x += dx;

        /* Labels need to be followed by a statement */
        (void)rc;
    }

    return OE_NONE;
}

int ocr_recognise(struct ocr *ocr, struct image *image) {
    size_t i;

    int rc;

    free(ocr->boundingboxes);
    ocr->boundingboxes = NULL;

    if((rc = find_bbs(ocr, image))) return rc;

    for(i=0;i<ocr->boundingbox_count;i++){
        struct ocr_boundingbox *bb;

        bb = ocr->boundingboxes+i;

        if((rc = process_line(ocr, bb, image))) return rc;
    }

    return OE_NONE;
}

void ocr_free(struct ocr *ocr) {
    font_renderer_free(&ocr->renderer);

    free(ocr->boundingboxes);
    ocr->boundingboxes = NULL;

    ocr->boundingbox_count = 0;
}
