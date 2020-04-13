#ifndef ERROR_H
#define ERROR_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
enum zu4_loglevel { ZU4_LOG_DBG, ZU4_LOG_INF, ZU4_LOG_WRN, ZU4_LOG_ERR };

void zu4_assert(bool exp, const char *fmt, ...);
void zu4_error(int level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
