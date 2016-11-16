#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "test_common.c"

#define MAX_BUF 65536


size_t shell_escape(char *dest, const char *source, size_t buflen, int *err)
{
// This escape function will definitely prevent accidents, but it's not
// guaranteed against malintent. No sir.
// Call with a NULL dest to retrieve the length needed.
#if !defined(_WIN32)
#define ENCAPS_QUOTE '\''
#define ESCAPED_QUOTE "'\\''"
#define ESCAPED_LEN 4
#else
#define ENCAPS_QUOTE '"'
#define ESCAPED_QUOTE "\"\\\"\""
#define ESCAPED_LEN 4
#endif
    const char *s = source;
    size_t len = strlen(source);
    size_t quotes = 0;
    // Work out how many quotes we need to escape so we know what to alloc
    while ((s = strchr(++s, ENCAPS_QUOTE))) {
        quotes++;
    }
    size_t escaped_len = (2 + (quotes * ESCAPED_LEN) + len) * sizeof(char);
    if (!dest)
        return escaped_len;
    if (buflen <= escaped_len) {
        *err = 1;
        return 0;
    }
    char *alias = dest;

    s = source;
    *alias++ = ENCAPS_QUOTE; // beginning quotes

    const char *next = s;
    do {
        next = strchr(s, ENCAPS_QUOTE);
        size_t tok_len = (next) ? (size_t)(next - s) : strlen(s);
        strncpy(alias, s, tok_len);
        alias += tok_len;
        if (!next) {
            break;
        }
        strcpy(alias, ESCAPED_QUOTE);
        alias += ESCAPED_LEN;
    } while ((s = next++));

    *alias = ENCAPS_QUOTE;
    assert(strlen(dest) == escaped_len);
    *err = 0;
    return strlen(dest);
}


int write_source_file(char *path, const char *text)
{
    int fd = mkstemp(path);
    assert(fd != -1);
    FILE *f = fdopen(fd, "wb");
    if ((size_t)fprintf(f, "%s", text) < strlen(text)) {
        fclose(f);
        return -1;
    };
    fflush(f);
    fclose(f);
    return 0;
}


int run_compiler(const char *target_path, const char *source_path,
                 const char *args)
{
    int ok, err;
    assert(source_path);
    assert(target_path);
    char cc_command[MAX_BUF];
    char source[MAX_BUF] = {'\0'};
    char target[MAX_BUF] = {'\0'};
    size_t source_len = shell_escape(source, source_path, MAX_BUF, &err);
    if (err)
        return -1;
    size_t target_len = shell_escape(target, target_path, MAX_BUF, &err);
    if (err)
        return -1;
    assert(source_len < MAX_BUF && target_len < MAX_BUF);
    if (source_len + target_len + strlen(args) + 32 < MAX_BUF) {
        ;
    }
    if (snprintf(
            cc_command,
            MAX_BUF,
            "cc -x c %s %s -o %s",
            args ? args : "",
            source,
            target
        ) >= MAX_BUF) {
            ok = -1;
            goto cleanup;
    }

    fprintf(stderr, "compiler command is '%s'\n", cc_command);
    ok = system(cc_command);
    if (ok != 0) {
        fprintf(
            stderr, "%s did not compile with the command '%s'\n",
            target, cc_command
        );
    }
cleanup:
    remove(source);
    return ok;
}


char *compile_string(const char *code, const char *args, const char *suffix)
{
#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif
    char source_path[MAX_BUF] = {'\0'};
    char target_path[MAX_BUF] = {'\0'};
    char cwd[MAX_BUF] = {'\0'};
    if (!getcwd(cwd, MAX_BUF)) {
        fprintf(stderr, "Couldn't get PWD\n");
        goto fail;
    };
    if (snprintf(source_path, MAX_BUF, "beemo_tmp_source-XXXXXX") >= MAX_BUF)
        goto fail;
    if (write_source_file(source_path, code) == -1)
        goto fail;
    if (snprintf(target_path, MAX_BUF, "%s%s%s.%s", cwd, PATH_SEP, source_path,
                 suffix) >= MAX_BUF)
        goto fail;
    if (run_compiler(target_path, source_path, args) != 0)
        goto fail;

    remove(source_path);
    return strdup(target_path);

fail:
    if (strlen(source_path))
        remove(source_path);
    return NULL;
}
