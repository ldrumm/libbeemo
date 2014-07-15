#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "error.h"
char * 
bmo_strdupb(const char * txt, size_t len) 
//this is essentially strdup but supporting binary arrays and requires a len param as a consequence
{
    assert(txt);
    char * new_txt = malloc(len);
    if(!new_txt){
        bmo_err("couldn't duplicate string:%s\n", strerror(errno));
        return NULL;
    }
    return memcpy(new_txt, txt, len);
}

char *
bmo_strdup(const char * txt)
{
//we supply a strdup-like functionality because the strdup family are non-portable
    size_t len = strlen(txt) + 1;
    char * s = malloc(len);
    if(!s){
        bmo_err("couldn't duplicate string:%s\n", strerror(errno));
        return NULL;
    }
    return memcpy(s, txt, len);
}


