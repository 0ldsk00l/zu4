/*
 * $Id: error.cpp 1916 2004-05-24 17:10:36Z dougday $
 */


#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "error.h"

/*
 * no GUI error functions: errors go to standard error stream
 */

void errorFatal(const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "xu4: error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(1);
}

void errorWarning(const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "xu4: warning: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
