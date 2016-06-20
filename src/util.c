#include "atomics.h"
#include "error.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

uint64_t bmo_uid(void)
{
    static volatile uint64_t i = 0;
    return BMO_ATOM_INC(&i);
}


char *bmo_strdupb(const char *txt, size_t len)
// this is essentially strdup but supporting binary arrays and requires a len
// param as a consequence
{
    assert(txt);
    char *new_txt = malloc(len);
    if (!new_txt) {
        bmo_err("couldn't duplicate string:%s\n", strerror(errno));
        return NULL;
    }
    return memcpy(new_txt, txt, len);
}


char *bmo_strdup(const char *txt)
{
    // we supply a strdup-like functionality because the strdup family are
    // non-portable
    size_t len = strlen(txt);
    char *s = malloc(len + 1);
    if (!s) {
        bmo_err("couldn't duplicate string:%s\n", strerror(errno));
        return NULL;
    }
    s[len] = '\0';
    return memcpy(s, txt, len);
}

