#ifndef ERROR_H
#define ERROR_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
enum xu4_loglevel { XU4_LOG_DBG, XU4_LOG_INF, XU4_LOG_WRN, XU4_LOG_ERR };

void xu4_assert(bool exp, const char *fmt, ...);
void xu4_error(int level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
