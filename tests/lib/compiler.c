#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define BMO_TEST_COMPILER_BUF 65536

int
write_source_file(char *path, const char *text)
{
    int fd = mkstemp(path);
    assert(fd != -1);
    FILE * f = fdopen(fd, "wb");
    if((size_t)fprintf(f, "%s",  text) < strlen(text)){
        fclose(f);
        return -1;
    };
    fflush(f);
    fclose(f);
    return 0;
}

int
run_compiler(const char *target, const char *source, const char *args)
{
    int ok;
    assert(source);
    assert(target);
    char compiler_command[BMO_TEST_COMPILER_BUF];
    snprintf(compiler_command,
        BMO_TEST_COMPILER_BUF,
        "gcc -x c %s %s -o %s",
        args ? args : "",
        source,
        target
    );
    fprintf(stderr, "compiler command is '%s'\n", compiler_command);
    ok = system(compiler_command);
    if(!ok==0){
        fprintf(stderr, "%s did not compile with the command '%s'\n",
            target, compiler_command
        );
    }
    remove(source);
    return ok;
}

char * compile_string(const char *code, const char *args, const char *suffix){
    char source_path[BMO_TEST_COMPILER_BUF];
    char target_path[BMO_TEST_COMPILER_BUF];
    char cwd[BMO_TEST_COMPILER_BUF];
    if(!getcwd(cwd, BMO_TEST_COMPILER_BUF)){
        fprintf(stderr, "couldn't get PWD\n");
        return NULL;
    };
    snprintf(source_path, BMO_TEST_COMPILER_BUF, "beemo_tmp_source-XXXXXX");
    if(write_source_file(source_path, code) == -1){
        remove(source_path);
        return NULL;
    }
    snprintf(target_path, BMO_TEST_COMPILER_BUF, "%s/%s.%s", cwd, source_path, suffix);
    if(run_compiler(target_path, source_path, args) != 0){
        remove(source_path);
        return NULL;
    }
    return strdup(target_path);
}
