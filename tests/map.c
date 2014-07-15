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
    FILE * f = fopen(path, "wb");
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
    FILE * f;
    char source[BUF_LEN];
    char sourcename[BUF_LEN];
    
    char command[BUF_LEN];
    
    int ok;
    snprintf(sourcename, BUF_LEN, "%s.c", target);
    f = fopen(sourcename, "w");
    
    snprintf(
        source, 
        BUF_LEN,
        "#include <stdio.h>\n"\
            "%s void %s(int *i){"\
            "*i *= 2;"
            "}\n\n", 
        export,
        funcname
    );
    printf("SOURCE:\n%s\n", source);
    snprintf(command, 
        BUF_LEN,
        "gcc -shared -fPIC %s -o %s",
        sourcename,
        target
    );
    printf("command is %s\n", command);
    fwrite(source, strlen(source), sizeof(char), f);
    fflush(f);
    ok = system(command);
    if(!ok==0){
        fprintf(stderr, "%s did not compile with the comand '%s'\n", target, command);
        return -1;
    }
    remove(sourcename);
    return ok;
}

int 
main(void)
{   
/*    assert(0);*/
    bmo_verbosity(BMO_MESSAGE_DEBUG);
    int failure = 0;
    char template[BUF_LEN];
    strcpy(template, "test_map-XXXXXX");
    if(test_file(template, 0x0f, FILE_LEN) == -1){
        failure = 1;
        goto cleanup;
    }
    //basic sanity tests for bmo_map()
    size_t size = bmo_fsize(template, &failure);
    assert(size == FILE_LEN);
    char * map = bmo_map(template, 0, 0);
    assert(map != NULL);
    for(size_t i = 0; i < size; i++){
        assert(map[i] == 0x0f);
    }    
    bmo_unmap(map, size);
    
    test_func("hello", "test_func");
    void (*func)(int *);
    func = bmo_loadlib("./hello", "test_func");
    assert(func != NULL);
    int i = 2;
    func(&i);
    assert(i == 4);
cleanup:
    remove(template);
    remove("hello");
    return failure;
}
