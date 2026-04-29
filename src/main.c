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

#include "image.h"

void e(void) {
    getchar();
}

int main(int argc, char **argv) {
    struct image img;
    int rc;

    size_t x, y;

    atexit(e);

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
