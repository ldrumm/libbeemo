#define _XOPEN_SOURCE 500 //mkstemp, fdopen

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "lib/compiler.c"

#include "../src/memory/map.h"
#include "../src/error.h"

#define BUF_LEN 1024
#define FILE_LEN 8192

int
test_file(char * path, char bytepattern, size_t len)
{
    char pattern = bytepattern;
    int fd = mkstemp(path);
    assert(fd != -1);
    FILE * f = fdopen(fd, "wb");
    if(!f){
        return -1;
    }
    for(size_t i = 0; i < len; i++){
        if(fwrite(&pattern, sizeof(pattern), 1, f) < 1){
            fclose(f);
            return -1;
        };
    }
    fflush(f);
    fclose(f);
    return 0;
}

char *
test_func(const char * funcname)
{

    #ifdef WIN32
    char export[] = "__declspec(dllimport) ";
    #else
    char export[] = "";
    #endif
    char source[BUF_LEN];
    snprintf(
        source,
        BUF_LEN,

        "#include <stdio.h>\n"
        "%s void %s(int *i){\n"
            "*i *= 2;\n"
            "printf(\"hello fromtes\");\n"
            "}\n\n",
        export,
        funcname
    );
    bmo_info("building test shared-lib\n");
    return compile_string(source, "-fPIC -shared", "so");
}

int
main(void)
{
    bmo_verbosity(BMO_MESSAGE_DEBUG);

    int failure = 0;
    char * shared_obj_path = NULL;
    char map_path[BUF_LEN];
    strcpy(map_path, "test_map-XXXXXX");
    if(test_file(map_path, 0x0f, FILE_LEN) == -1){
        failure = 1;
        goto cleanup;
    }
    //basic sanity tests for bmo_map()
    size_t size = bmo_fsize(map_path, &failure);
    assert(size == FILE_LEN);
    char * map = bmo_map(map_path, 0, 0);
    assert(map != NULL);
    for(size_t i = 0; i < size; i++){
        assert(map[i] == 0x0f);
    }
    bmo_unmap(map, size);

    /*******************can we load symbols?*******************************/
    shared_obj_path = test_func("test_func");
    if(!shared_obj_path){
        assert("couldn't compile test function'" && 0);
        return 1;
    };
    void (*func)(int *);
    func = bmo_loadlib(shared_obj_path, "test_func");
    assert(func != NULL);
    int i = 2;
    func(&i);
    assert(i == 4);

cleanup:
    remove(map_path);
    remove(shared_obj_path);
    free(shared_obj_path);
    return failure;
}
