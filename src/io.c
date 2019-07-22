/*
 * io.c
 * Copyright (C) 2004 Doug Day
 * Copyright (C) 2005 Andrew Taylor
 * Copyright (C) 2019 R. Danbrook
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */
#include <stdio.h>
#include <stdint.h>

#include "io.h"

int writeInt(uint32_t i, FILE *f) {
    if (fputc(i & 0xff, f) == EOF ||
        fputc((i >> 8) & 0xff, f) == EOF ||
        fputc((i >> 16) & 0xff, f) == EOF ||
        fputc((i >> 24) & 0xff, f) == EOF)
        return 0;
    return 1;
}

int writeShort(uint16_t s, FILE *f) {
    if (fputc(s & 0xff, f) == EOF ||
        fputc((s >> 8) & 0xff, f) == EOF)
        return 0;
    return 1;
}

int writeChar(uint8_t c, FILE *f) {
    if (fputc(c, f) == EOF)
        return 0;
    return 1;
}

int readInt(uint32_t *i, FILE *f) {
    *i = fgetc(f);
    *i |= (fgetc(f) << 8);
    *i |= (fgetc(f) << 16);
    *i |= (fgetc(f) << 24);
    
    return 1;
}

int readShort(uint16_t *s, FILE *f) {
    *s = fgetc(f);
    *s |= (fgetc(f) << 8);

    return 1;
}

int readChar(uint8_t *c, FILE *f) {
    *c = fgetc(f);

    return 1;
}
