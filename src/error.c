#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"

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
#ifdef __cplusplus
}
#endif
