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
#include <string.h>
#include <locale.h>
#include <stdio.h>
#include <wchar.h>

#include "image.h"
#include "font.h"
#include "ocr.h"

#define MAIN_DEBUG_IMAGE    0
#define MAIN_DEBUG_FONT     0

void e(void) {
    getchar();
}

#if MAIN_DEBUG_IMAGE
static int debug_image(int argc, char **argv) {
    struct image img;
    int rc;

    size_t x, y;

    (void)argc;
    (void)argv;

    if((rc = image_create(&img, 32, 32, 72))){
        puts(image_get_error_str(rc));
        return 1;
    }

    img.data[33] = IMAGE_RGBAINT(255, 255, 0, 255);

    if((rc = image_write(&img, "test.bmp", IT_BMP, 0))){
        puts(image_get_error_str(rc));

        return 1;
    }

    image_free(&img);

    if((rc = image_load(&img, "test.bmp"))){
        puts(image_get_error_str(rc));

        return 1;
    }

    for(y=0;y<img.h;y++){
        for(x=0;x<img.w;x++){
            fputc(img.data[y*img.w+x]&IMAGE_RGBAINT(255, 255, 255, 0) ?
                  '.' : '#', stdout);
            /*printf("%08x ", img.data[y*img.w+x]);*/
        }
        fputc('\n', stdout);
    }

    image_free(&img);

    return 0;
}

#elif MAIN_DEBUG_FONT
static int debug_font(int argc, char **argv) {
    struct font font;
    struct font_renderer renderer;
    struct image image;
    struct font_glyph *glyph;
    wchar_t *str;
    char *tmp;
    size_t len;
    size_t i;
    int char_len;
    int w;
    int rc;

    int size = 150;
    int dpi = 72;

    int x;

    setlocale(LC_CTYPE, "");

    if(argc < 4){
        fprintf(stderr, "USAGE: %s TTF_FILE CHAR OUPUT_IMAGE\n", *argv);

        return 1;
    }

    if((rc = font_load(&font, argv[1]))){
        puts(font_get_error_str(rc));

        return 1;
    }
    if((rc = font_renderer_init(&renderer, &font, dpi, size))){
        puts(font_get_error_str(rc));

        font_free(&font);

        return 1;
    }

    len = 0;
    tmp = argv[2];
    while((char_len = mblen(tmp, strlen(tmp))) > 0){
        len++;
        tmp += char_len;
    }
    if(char_len < 0 && strlen(tmp)){
        fputs("Invalid string!\n", stderr);

        return 1;
    }

    str = malloc((len+1)*sizeof(wchar_t));
    if(str == NULL){
        fputs("Can't allocate string!\n", stderr);

        font_renderer_free(&renderer);
        font_free(&font);

        return 1;
    }

    mbstowcs(str, argv[2], len);
    str[len] = 0;

    w = 0;
    for(i=0;str[i];i++){
        wchar_t c = str[i];

        /* TODO: Support UTF-8. */
        glyph = font_lookup_char(&font, c);

        if(glyph == NULL) glyph = font.glyphs;

        w += font_scale_size(&font, dpi, size, glyph->advance_width);
    }

    if((rc = image_create(&image, w, renderer.h, dpi))){
        puts(image_get_error_str(rc));

        font_renderer_free(&renderer);
        font_free(&font);

        return 1;
    }

    memset(image.data, 0xFF, image.w*image.h*sizeof(pixel_t));

    x = 0;
    for(i=0;str[i];i++){
        wchar_t c = str[i];

        /* TODO: Support UTF-8. */
        glyph = font_lookup_char(&font, c);

        if(glyph == NULL){
            fputs("Glyph not found!\n", stderr);

            glyph = font.glyphs;
        }

        font_renderer_glyph(&renderer, &font, glyph, size);
        font_renderer_to_image(&renderer, &image, renderer.x+x+
                               font_scale_size(&font, dpi, size,
                                               glyph->left_side_bearing),
                               font_scale_size(&font, dpi, size,
                                               font.max_ascender+font.line_gap-
                                               font.max_descender)-
                               renderer.baseline);

        x += font_scale_size(&font, dpi, size, glyph->advance_width);
    }

    if((rc = image_write(&image, argv[3], IT_BMP, 0))){
        puts(image_get_error_str(rc));

        font_renderer_free(&renderer);
        font_free(&font);
        image_free(&image);

        return 1;
    }

    font_renderer_free(&renderer);
    font_free(&font);
    image_free(&image);

    return 0;
}
#else
static int ocr(int argc, char **argv) {
    struct font font;
    struct image image;
    struct image out;
    struct ocr ocr;

    int rc;

    int size = 200;

    if(argc < 3){
        fprintf(stderr, "USAGE: %s FONT IMAGE\n", *argv);
    }

    if((rc = font_load(&font, argv[1]))){
        puts(font_get_error_str(rc));

        return 1;
    }
    if((rc = image_load(&image, argv[2]))){
        puts(image_get_error_str(rc));

        font_free(&font);

        return 1;
    }

    if((rc = image_create(&out, image.w, image.h, image.ppi))){
        puts(image_get_error_str(rc));

        font_free(&font);

        return 1;
    }

    if((rc = ocr_init(&ocr, &font, image.ppi, size))){
        puts(ocr_get_error_str(rc));

        image_free(&image);
        font_free(&font);

        return 1;
    }

    if((rc = ocr_recognise(&ocr, &image))){
        puts(ocr_get_error_str(rc));

        ocr_free(&ocr);
        image_free(&image);
        font_free(&font);

        return 1;
    }

    memcpy(out.data, image.data, image.w*image.h*sizeof(pixel_t));

    {
        size_t i;

        for(i=0;i<ocr.boundingbox_count;i++){
            struct ocr_boundingbox *bb = ocr.boundingboxes+i;

            size_t n;

            for(n=bb->x1;n<bb->x2;n++){
                out.data[bb->y1*out.w+n] = IMAGE_RGBAINT(255, 0, 0, 0);
                out.data[bb->y2*out.w+n] = IMAGE_RGBAINT(255, 0, 0, 0);
            }
            for(n=bb->y1;n<bb->y2;n++){
                out.data[n*out.w+bb->x1] = IMAGE_RGBAINT(255, 0, 0, 0);
                out.data[n*out.w+bb->x2] = IMAGE_RGBAINT(255, 0, 0, 0);
            }
        }
    }

    if((rc = image_write(&out, "out.bmp", IT_BMP, 0))){
        puts(image_get_error_str(rc));

        font_free(&font);
        image_free(&image);

        return 1;
    }

    ocr_free(&ocr);
    image_free(&image);
    font_free(&font);

    return 0;
}

#endif

int main(int argc, char **argv) {
#if (defined(_WIN32) || defined(_WIN64)) && DEBUG
    atexit(e);
#endif

#if MAIN_DEBUG_IMAGE
    return debug_image(argc, argv);
#elif MAIN_DEBUG_FONT
    return debug_font(argc, argv);
#else
    return ocr(argc, argv);
#endif
}
