#include "utils.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

void die(const char *fmt, ...) {
    va_list ap;
    int saved_errno;

    saved_errno = errno;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fmt[0] && fmt[strlen(fmt)-1] == ':')
        fprintf(stderr, " %s", strerror(saved_errno));
    fputc('\n', stderr);

    exit(1);
}

int power_of_2_ceil(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    if (n < 4) { n = 4; }
    return n;
}

DEFINE_BUFFER(Error, error)

error new_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char* err = NULL;
    if (vasprintf(&err, format, args) == -1) die("new_error: vasprintf");
    va_end(args);
    return err;
}


error error_num_args(const char *name, int expected, int actual) {
    return new_error("%s takes %d argument%s got %d",
            name, expected, expected != 1 ? "s" : "", actual);
}
