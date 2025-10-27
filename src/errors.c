#include "errors.h"

#include <stdarg.h>
#include <stdbool.h>

DEFINE_BUFFER(Error, Error)

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

void print_error(const char *input, Error *err) {
    Token tok = err->token;
    const char *cur = tok.start;

    // find distance from start of line to [err.token.start].
    int left_dist = 0;
    while (true) {
        if (cur == input || *(cur - 1) == '\n') {
            break;
        }
        cur--;
        left_dist++;
    }

    const char *start_of_line = cur;
    cur = tok.start + tok.length;

    // find distance from [err.token.start] to end of line.
    int right_dist = 0;
    char ch;
    while (true) {
        ch = *cur;
        if (ch == '\n' || ch == '\0') {
            break;
        }
        cur++;
        right_dist++;
    }

    printf("%s\n", err->message);

    // print line
    printf("%4d | %.*s\n", tok.line,
            left_dist + tok.length + right_dist,
            start_of_line);

    // highlight token
    left_dist += 7; // padding + ' | '
    int n;
    for (n = 0; n < left_dist; n++) { putc(' ', stdout); }
    for (n = 0; n < tok.length; n++) { putc('^', stdout); }
    putc('\n', stdout);
}

void free_error(Error *err) {
    free(err->message);
    free(err);
}
