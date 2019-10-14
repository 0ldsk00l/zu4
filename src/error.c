/*
 * error.c
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
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"

void xu4_assert(bool exp, const char *fmt, ...) {
	// Handle assertions
	va_list va;
	char buffer[256] = {0};
	
	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);
	if (!exp) {
		fprintf(stderr, "Assertion failed: %s\n", buffer);
		fflush(stderr);
		abort();
	}
}

void xu4_error(int level, const char *fmt, ...) {
	// Handle and log errors
	va_list va;
	char buffer[4096] = {0};
	const char *levelchr = "diwe";
	
	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);
	
	fprintf(stderr, "%c: %s\n", levelchr[level], buffer);
	fflush(stderr);
	
	if (level >= XU4_LOG_ERR) { exit(1); }
}
