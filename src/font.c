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
#include <stdlib.h>
#include <string.h>

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

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#if TWOS_COMPLEMENT
#define SIGNED(a, b) (a)
#else
#define SIGNED(a, b) ((a)&(1<<(b-1)) ? ~(a)+1 : (a))
#endif

int font_load(struct font *font, char *file) {
    FILE *fp;

    struct glyph *glyphs;

    font_u32_t table_pos[FONT_REQUIRED_TABLES];
    font_u32_t table_count;

    font_u32_t glyph_count;

    font_s16_t xmin, ymin;
    font_s16_t xmax, ymax;

    unsigned short int style;

    unsigned short int glyph_recursion;

    unsigned short int units_per_em;

    unsigned char long_offsets;

    /* TODO: Verify the font's integrity */

    fp = fopen(file, "rb");
    if(fp == NULL){

        return FE_OPEN;
    }

    {
        /* Read the table directory */

        static const unsigned char required_tables[FONT_REQUIRED_TABLES][4] = {
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

        unsigned char b[MAX(4+2+3*2, 4*4)];

        font_u32_t i;

        unsigned short int found_tables = 0;

        if(fread(b, 1, 4+2+3*2, fp) != 4+2+3*2){
            fclose(fp);

            return FE_READ;
        }

        table_count = (b[4]<<8)|b[5];

        for(i=0;i<table_count;i++){
            size_t n;

            if(fread(b, 1, 4*4, fp) != 4*4){
                fclose(fp);

                return FE_READ;
            }

            for(n=0;n<FONT_REQUIRED_TABLES;n++){
                if(!memcmp(b, required_tables[n], 4)){
                    found_tables++;

                    table_pos[n] = (b[8]<<24)|(b[9]<<16)|(b[10]<<8)|b[11];

                    break;
                }
            }
        }

        if(found_tables != FONT_REQUIRED_TABLES){
            fclose(fp);

            return FE_MISSING_TABLES;
        }
    }

    {
        /* Read the maxp table */

        unsigned char b[2*4];

        if(fseek(fp, table_pos[FONT_MAXP], SEEK_SET)){
            fclose(fp);

            return FE_SEEK;
        }

        if(fread(b, 1, 2*4, fp) != 2*4){
            fclose(fp);

            return FE_READ;
        }

        if(b[0] != 0 || b[1] != 1 || b[2] != 0 || b[3] != 0){
            fclose(fp);

            return FE_CORRUPTED_MAXP;
        }

        glyph_count = (b[4]<<24)|(b[5]<<16)|(b[6]<<8)|b[7];

        if(fseek(fp, 12*2, SEEK_CUR)){
            fclose(fp);

            return FE_SEEK;
        }

        if(fread(b, 1, 2, fp) != 2){
            fclose(fp);

            return FE_READ;
        }

        glyph_recursion = (b[0]<<8)|b[1];
    }

    /* Allocate memory to store the glyph structures */
    glyphs = malloc(glyph_count*sizeof(struct glyph));
    if(glyphs == NULL){
        fclose(fp);

        return FE_MEM;
    }

    {
        /* Read the head table */

        unsigned char b[MAX(4, MAX(10, 5*2))];

        if(fseek(fp, table_pos[FONT_HEAD], SEEK_SET)){
            fclose(fp);

            return FE_SEEK;
        }

        if(fread(b, 1, 4, fp) != 4){
            fclose(fp);

            return FE_READ;
        }
        if(b[0] != 0 || b[1] != 1 || b[2] != 0 || b[3] != 0){
            fclose(fp);

            return FE_CORRUPTED_HEAD;
        }

        if(fseek(fp, 2*4, SEEK_CUR)){
            fclose(fp);

            return FE_SEEK;
        }

        if(fread(b, 1, 10, fp) != 10){
            fclose(fp);

            return FE_READ;
        }
        printf("0x%02x%02x%02x%02x\n", b[0], b[1], b[2], b[3]);
        if(b[0] != 0x5F || b[1] != 0x0F || b[2] != 0x3C || b[3] != 0xF5){
            fclose(fp);

            return FE_CORRUPTED_HEAD;
        }

        if(b[5]&1){
            font->vertical = 0;
        }else if(b[5]&(1<<6)){
            font->vertical = 1;
        }

        units_per_em = (b[8]<<8)|b[9];

        if(fseek(fp, 2*8, SEEK_CUR)){
            fclose(fp);

            return FE_SEEK;
        }

        if(fread(b, 1, 5*2, fp) != 5*2){
            fclose(fp);

            return FE_READ;
        }

        xmin = SIGNED((b[0]<<8)|b[1], 16);
        ymin = SIGNED((b[2]<<8)|b[3], 16);
        xmax = SIGNED((b[4]<<8)|b[5], 16);
        ymax = SIGNED((b[6]<<8)|b[7], 16);

        style = (b[8]<<8)|b[9];

        if(fseek(fp, 2*2+1, SEEK_CUR)){
            fclose(fp);

            return FE_SEEK;
        }

        if(fread(&long_offsets, 1, 1, fp) != 1){
            fclose(fp);

            return FE_READ;
        }
    }

    puts("head loaded");

    {
        /* Read the loca table */

        if(fseek(fp, table_pos[FONT_LOCA], SEEK_SET)){
            fclose(fp);

            return FE_SEEK;
        }

        if(long_offsets){
            font_u32_t i;

            for(i=0;i<glyph_count;i++){
                unsigned char b[4];

                /* TODO: Read multiple offsets at once */
                if(fread(b, 1, 4, fp) != 4){
                    fclose(fp);

                    return FE_READ;
                }

                glyphs[i].offset = (b[0]<<24)|(b[1]<<16)|(b[2]<<8)|b[3];
            }
        }else{
            font_u32_t i;

            for(i=0;i<glyph_count;i++){
                unsigned char b[2];

                /* TODO: Read multiple offsets at once */
                if(fread(b, 1, 2, fp) != 2){
                    fclose(fp);

                    return FE_READ;
                }

                glyphs[i].offset = (b[0]<<8)|b[1];
            }
        }
    }

    puts("loca loaded");

    {
        /* Load all the glyphs from the glyf table */

        font_u32_t *glyph_stack;
        font_u32_t cur = 0;

        font_u32_t i;

        if(glyph_recursion){
            glyph_stack = malloc(glyph_recursion*sizeof(font_u32_t));
            if(glyph_stack == NULL){
                fclose(fp);

                return FE_MEM;
            }
        }

        for(i=0;i<glyph_count;i++){
            glyphs[i].loaded = 0;
            glyphs[i].waits_loading = 0;
        }

        i = 0;
        while(i < glyph_count || cur){
            unsigned char b[2+4*2];

            font_s16_t contour_count;

            if(glyphs[i].loaded) continue;

            if(fread(b, 1, 2+4*2, fp) != 2+4*2){
                fclose(fp);

                return FE_READ;
            }

            contour_count = SIGNED((b[0]<<8)|b[1], 16);

            glyphs[i].xmin = SIGNED((b[2]<<8)|b[3], 16);
            glyphs[i].ymin = SIGNED((b[4]<<8)|b[5], 16);
            glyphs[i].xmax = SIGNED((b[6]<<8)|b[7], 16);
            glyphs[i].ymax = SIGNED((b[8]<<8)|b[9], 16);

            if(contour_count >= 0){
                /* It is a simple glyph */

                /**/
            }else{
                /* It is a compound glyph */

                glyphs[i].waits_loading = 0;
            }

            if(cur){
                i = glyph_stack[--cur];
            }else{
                i++;
            }
LOAD:
            (void)b; /* Labels need to be followed by statements. */
        }
    }

    fclose(fp);

    return FE_NONE;
}

void font_free(struct font *font) {

}
