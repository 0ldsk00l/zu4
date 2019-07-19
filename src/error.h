/*
 * $Id: error.h 2475 2005-08-22 05:46:10Z andrewtaylor $
 */

#ifndef ERROR_H
#define ERROR_H

#if __GNUC__
#define PRINTF_LIKE(x,y)  __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTF_LIKE(x,y) /* nothing */
#endif

void errorFatal(const char *fmt, ...) PRINTF_LIKE(1, 2);
void errorWarning(const char *fmt, ...) PRINTF_LIKE(1, 2);

#endif
