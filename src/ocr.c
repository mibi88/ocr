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

#define OCR_IMPL

#include "ocr.h"

#define STR(s) #s,
#define STR_LAST(s) #s

char *ocr_get_error_str(int error) {
    char *errors[] = {
        OCR_ERROR_X(STR, STR_LAST)
    };

    return errors[error];
}

int ocr_init(struct ocr *ocr, struct font *font, int dpi, int max_size) {
    if(font_renderer_init(&ocr->renderer, font, dpi, max_size)){
        return OE_RENDERER_INIT;
    }
    ocr->font = font;
    ocr->boundingboxes = NULL;

    ocr->line_treshold = 0;
    ocr->char_treshold = 0;
    ocr->min_height = 5;

    ocr->boundingbox_count = 0;

    return OE_NONE;
}

static int ocr_find_bbs(struct ocr *ocr, struct image *image){
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

int ocr_recognise(struct ocr *ocr, struct image *image) {
    int rc;

    free(ocr->boundingboxes);
    ocr->boundingboxes = NULL;

    if((rc = ocr_find_bbs(ocr, image))) return rc;

    return OE_NONE;
}

void ocr_free(struct ocr *ocr) {
    font_renderer_free(&ocr->renderer);

    free(ocr->boundingboxes);
    ocr->boundingboxes = NULL;

    ocr->boundingbox_count = 0;
}
