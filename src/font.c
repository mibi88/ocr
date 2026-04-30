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
#include <stdio.h>

#define FONT_IMPL

#include "font.h"

#define STR(s) #s,
#define STR_LAST(s) #s

char *font_get_error_str(int error) {
    char *errors[] = {
        FONT_ERROR_X(STR, STR_LAST)
    };

    return errors[error];
}

int font_load(struct font *font, char *file) {
    FILE *fp;

    fp = fopen(file, "rb");
    if(fp == NULL){

        return FE_OPEN;
    }

    {
        /* Load the directory */
        static const unsigned char required_tables[][4] = {
            {0x63, 0x6d, 0x61, 0x70}, /* cmap */
            {0x67, 0x6c, 0x79, 0x66}, /* glyf */
            {0x68, 0x65, 0x61, 0x64}, /* head */
            {0x68, 0x68, 0x65, 0x61}, /* hhea */
            {0x68, 0x6d, 0x74, 0x78}, /* hmtx */
            {0x6c, 0x6f, 0x63, 0x61}, /* loca */
            {0x6d, 0x61, 0x78, 0x70}, /* maxp */
            {0x6e, 0x61, 0x6d, 0x65}, /* name */
            {0x70, 0x6f, 0x73, 0x74}, /* post */
        };

        unsigned char b[4+2+3*2];

        font_u32_t table_count;
        unsigned short int found_tables;

        font_u32_t i;

        if(fread(b, 1, 4+2+3*2, fp) != 4+2+3*2){
            fclose(fp);

            return FE_READ;
        }

        table_count = (b[4]<<24)|(b[5]<<16)|(b[6]<<8)|b[7];

        for(i=0;i<FONT_REQUIRED_TABLES;i++){
            /* TODO */
        }
    }

    fclose(fp);

    return FE_NONE;
}

void font_free(void) {

}
