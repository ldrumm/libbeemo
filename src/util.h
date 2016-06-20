#ifndef BMO_UTIL_H
#define BMO_UTIL_H
#include <stddef.h>
#include <stdint.h>

uint64_t bmo_uid(void);
char *bmo_strdupb(const char *txt, size_t len);
char *bmo_strdup(const char *txt);

#endif

