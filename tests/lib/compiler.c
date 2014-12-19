#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "test_common.c"

#define BMO_TEST_COMPILER_BUF 65536


char *shell_escape(const char * source)
{
    // This escape function will definitely prevent accidents, but it's not
    //guaranteed against malintent.
    const char *s = source;
    size_t len = strlen(source);
    size_t quotes = 0;
    //work out how many single quotes we need to escape so we know what to alloc
    while((s = strchr(++s, '\'')) ){
        quotes++;
    }
    char *escaped = calloc(sizeof(char), 2 + (quotes * 3) + len + 1);
    char *alias = escaped;
    if(!escaped)
        return NULL;

    s = source;
    *alias++ = '\'';//beginning quotes

    const char *next = s;
    size_t tok_len;
    do{
        next = strchr(s, '\'');
        tok_len = (next) ? (size_t)(next - s): strlen(s);
        strncpy(alias, s, tok_len);
        alias += tok_len;
        if(!next){
            break;
        }
        strcpy(alias, "'\\''");
        alias+=4;
    }while((s = ++next));
    *alias = '\'';

    return escaped;
}

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
run_compiler(const char *target_path, const char *source_path, const char *args)
{
    int ok;
    assert(source_path);
    assert(target_path);
    char compiler_command[BMO_TEST_COMPILER_BUF];
    char *source = shell_escape(source_path);
    char *target = shell_escape(target_path);
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
    free(source);
    free(target);
    return ok;
}

char * compile_string(const char *code, const char *args, const char *suffix)
{
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
    remove(source_path);
    return strdup(target_path);
}
