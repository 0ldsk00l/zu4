/*
 * xmlparse.c
 * Copyright (C) 2020 R. Danbrook
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

// This code is immature and only useful for simple parsing currently.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "yxml.h"

#include "xmlparse.h"

#define BUFSIZE 1024

static void *buf;	// Buffer
static yxml_t *x;	// Uninitialized state
static char *doc;	// Null-terminated XML document
static char *ptr;

void xu4_xmlparse_init(const char *filename) {
	buf = malloc(BUFSIZE);
	x = malloc(sizeof(yxml_t));
	yxml_init(x, buf, BUFSIZE);
	
	FILE *file = fopen(filename, "r");
	if (!file) { return; }
	
	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	doc = malloc(filesize * sizeof(char));
	if (doc == NULL) { return; }
	ptr = doc;
	
	size_t result = fread(doc, sizeof(char), filesize, file);
	if (result != filesize) { return; }
	fclose(file);
}

void xu4_xmlparse_deinit() {
	free(buf);
	free(x);
	free(doc);
}

int xu4_xmlparse_find(char *value, const char *elemname, const char *attrname) {
	int index = 0;
	while(*ptr) {
		yxml_ret_t r = yxml_parse(x, *ptr);
		if (r < 0) break;
		
		switch(r) {
			case YXML_ATTRVAL:
				if ((!strcmp(x->elem, elemname)) && (!strcmp(x->attr, attrname))) {
					value[index++] = *ptr;
				}
				break;
			case YXML_ATTREND:
				if ((!strcmp(x->elem, elemname)) && (!strcmp(x->attr, attrname))) {
					value[index] = '\0';
					ptr++;
					return 1;
				}
				break;
			default: break;
		}
		ptr++;
	}
	return 0;
}
