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
#include "image.h"

#include <stdio.h>
#include <string.h>

#define STR(s) #s

char *image_get_error_str(int error) {
    char *errors[] = {
        STR(IE_SUCCESS),
        STR(IE_NOT_FOUND),
        STR(IE_READ),
        STR(IE_SIG),
        STR(IE_UNSUPPORTED),
        STR(IE_UNKNOWN)
    };

    return errors[error];
}

int image_load(struct image *img, char *file) {
    FILE *fp;
    char sig[3] = "\0\0";
    unsigned char b[4];

    size_t size;
    size_t data_offset;
    size_t header_size;

    fp = fopen(file, "rb");

    if(fp == NULL){
        return IE_NOT_FOUND;
    }

    if(fread(sig, 1, 2, fp) != 2){
        fclose(fp);

        return IE_READ;
    }

    if(strcmp(sig, "BM")){
        fclose(fp);

        return IE_SIG;
    }

    if(fread(b, 1, 4, fp) != 4){
        fclose(fp);

        return IE_READ;
    }

    size = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

    if(fseek(fp, 4, SEEK_CUR)){
        fclose(fp);

        return IE_READ;
    }

    if(fread(b, 1, 4, fp) != 4){
        fclose(fp);

        return IE_READ;
    }

    data_offset = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

    if(fread(b, 1, 4, fp) != 4){
        fclose(fp);

        return IE_READ;
    }

    header_size = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

#if 0
    printf("%lu, %lu, %lu\n", size, data_offset, header_size);
#endif

    switch(header_size){
        case 40:
            /* BITMAPINFOHEADER */

            {
                size_t width;
                size_t height;
                unsigned short int color_planes;
                unsigned short int bpp;
                size_t compression;
                size_t raw_size;
                size_t hpxpm;
                size_t vpxpm;
                size_t colors;
                size_t important;

                /* 4 byte width */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                width = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

                /* 4 byte height */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                height = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

                /* 2 byte color plane count */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                color_planes = b[0]|(b[1]<<8);

                /* 2 byte bits per pixel */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                bpp = b[0]|(b[1]<<8);

                /* 4 byte compression method */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                compression = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

                /* 4 byte size of the raw image data (can be 0 for BI_RGB) */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                raw_size = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

                /* 4 byte horizontal pixel per metre */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                hpxpm = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

                /* 4 byte vertical pixel per metre */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                vpxpm = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

                /* 4 byte number of colors in the palette count
                 * (0 for 2^bit_count) */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                colors = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

                /* 4 byte number of important colors */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return IE_READ;
                }

                important = b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24);

                printf("width:          %lu\n"
                       "height:         %lu\n"
                       "color_planes:   %lu\n"
                       "bpp:            %lu\n"
                       "compression:    %lu\n"
                       "raw_size:       %lu\n"
                       "hpxpm:          %lu\n"
                       "vpxpm:          %lu\n"
                       "colors:         %lu\n"
                       "important:      %lu\n",
                       width, height, color_planes, bpp, compression,
                       raw_size, hpxpm, vpxpm, colors, important);
            }

            break;
        default:
            /* Unsupported bitmap format */
            break;
    }

    fclose(fp);

    return IE_SUCCESS;
}

int image_write(struct image *img, char *file, int type, int flags) {
    FILE *fp;

    fp = fopen(file, "w");

    fclose(fp);
}

void image_free(struct image *img) {
    /**/
}

