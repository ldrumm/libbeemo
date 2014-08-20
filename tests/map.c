#define _XOPEN_SOURCE 500 //mkstemp, fdopen

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
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

int
test_func(const char * target, const char * funcname)
{

    #ifdef WIN32
    char export[] = "__declspec(dllimport) ";
    #else
    char export[] = "";
    #endif
    FILE * source_file;
    char source_path[BUF_LEN];
    char compiler_command[BUF_LEN];
    int ok;

    snprintf(source_path, BUF_LEN-1, "%s.c", target);
    source_file = fopen(source_path, "w");
    assert(source_file);
    fprintf(
        source_file,
        "#include <stdio.h>\n"\
            "%s void %s(int *i){"\
            "*i *= 2;"
            "}\n\n",
        export,
        funcname
    );
    fflush(source_file);

    snprintf(compiler_command,
        BUF_LEN,
        "gcc -shared -fPIC %s -o %s",
        source_path,
        target
    );
    bmo_info("building test shared-lib with command '%s\n'", compiler_command);

    ok = system(compiler_command);
    if(!ok==0){
        fprintf(stderr, "%s did not compile with the command '%s'\n", target, compiler_command);
        return -1;
    }
    remove(source_path);
    return ok;
}

int
main(void)
{
    bmo_verbosity(BMO_MESSAGE_DEBUG);

    int failure = 0;
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

    if(!test_func("hello", "test_func") == 0){
        assert("couldn't compile test function'" && 0);
        return 1;
    };
    void (*func)(int *);
    func = bmo_loadlib("./hello", "test_func");
    assert(func != NULL);
    int i = 2;
    func(&i);
    assert(i == 4);
cleanup:
    remove(map_path);
    remove("hello");
    return failure;
}
