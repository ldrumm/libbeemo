#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "definitions.h"
#include "error.h"

static void default_logger(const char *, const char *, uint32_t, void *);
static uint32_t debug_level = BMO_MESSAGE_INFO;
static char msg_buf[BMO_ERR_LEN] = {'\0'};
static void *err_fn_userdata = NULL;
static void (*err_fn)(
    const char *, const char *, uint32_t, void *
) = default_logger;


void bmo_verbosity(uint32_t level)
{
    if (level > BMO_MESSAGE_DEBUG)
        level = BMO_MESSAGE_DEBUG;
    debug_level = level;
}

/**
    Set the function used to handle all messages from the library.
    If ``callback`` is NULL, only return the currently registered callback
    otherwise set the error callback to the given function, and return that
    function.
    If an error function has been set by calling this function with an
    appropriate argument it will be called with the error message, the name of
    the function on which the error occurred, the error level (one of
    BMO_MESSAGE_CRIT, BMO_MESSAGE_DEBUG, BMO_MESSAGE_INFO, BMO_MESSAGE_NONE) and
    the supplied userdata. If userdata is not required, pass `NULL` for this
    argument.

    The given callback will only be called if the message's level exceeds the
    currently set verbosity.

    Using this function replaces the internal logging (i.e. to file or stderr)
    so the given function must assume those duties if required.
*/

// Function of 2 args returning a void function of 4 args
void (*bmo_set_logger(
    void (*callback)(const char *, const char *, uint32_t, void *), void *userdata))
    (const char *, const char *, uint32_t, void *)
{
    if (!callback) {
        return err_fn;
    }
    err_fn = callback;
    err_fn_userdata = userdata;

    return err_fn;
}

void bmo_set_logger_data(void *userdata)
{
    err_fn_userdata = userdata;
}

BMO_DEPRECATED FILE *bmo_err_stream(FILE *fp)
{
    // set or get the current output stream.
    // If called with NULL, simply returns a pointer to the existing stream
    if (!fp) return err_fn_userdata;
    bmo_set_logger_data(fp);
    return err_fn_userdata;
}

static void default_logger(
    const char *msg,
    const char *func,
    uint32_t level,
    void *userdata
)
{
    const char *color;
    switch (level) {
        case BMO_MESSAGE_CRIT: color = ANSI_COLOR_RED; break;
        case BMO_MESSAGE_INFO: color = ANSI_COLOR_YELLOW; break;
        case BMO_MESSAGE_DEBUG: color = ANSI_COLOR_GREEN; break;
        default: color = "";
    }
    char buf[BMO_ERR_LEN] ={'\0'};
    snprintf(buf, sizeof buf - 1, "[ %s%s()%s ]: %s", color, func,
             ANSI_COLOR_RESET, msg);
    buf[sizeof buf - 1] = '\0';
    fputs(buf, userdata ? userdata : stderr);
}

// Print a message to the current log if `level` is above the configured loglevel
// `fn` is the function name, message is a standard printf-conformant format
// string. In most cases, the macros bmo_debug, bmo_warning, bmo_err are more
// convenient.
void _bmo_message(uint32_t level, const char *fn, const char *message, ...)
{
    if (!message || !err_fn)
        return;

    va_list argp;
    va_start(argp, message);
    int doprint = 1;

    char buf[BMO_ERR_LEN] = {'\0'};
    const size_t max = sizeof buf - 1;
    if (level <= debug_level) {
        vsnprintf(buf, max, message, argp);
        err_fn(buf, fn, level, err_fn_userdata);
    } else if (level == BMO_MESSAGE_CRIT) {
        if (doprint)
            vsnprintf(buf, max, message, argp);
    } else {
        goto end;
    }
    strncpy(msg_buf, buf, max);

end:
    va_end(argp);
}

const char *bmo_strerror(void)
{
    return msg_buf;
}
