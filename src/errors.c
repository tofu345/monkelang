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

void highlight_token(Token tok) {
    const char *cur = tok.start;

    // distance from start of line to [token.start].
    int offset = tok.position;
    for (; offset > 0; offset--) {
        // walk left until start of source code or end of previous line.
        if (*(cur - 1) == '\n') {
            break;
        }
        cur--;
    }
    int left_dist = tok.position - offset;

    const char *start_of_line = cur;
    cur = tok.start + tok.length;

    // distance from [token.start] to end of line.
    int right_dist = 0;
    while (true) {
        // walk right until end of source code or end of current line.
        char ch = *cur;
        if (ch == '\0' || ch == '\n') {
            break;
        }
        cur++;
        right_dist++;
    }

    // print line
    printf("\n%4d | %.*s\n", tok.line,
            left_dist + tok.length + right_dist,
            start_of_line);

    // highlight token
    left_dist += 7; // + 'line | '
    int n;
    for (n = 0; n < left_dist; n++) { putc(' ', stdout); }
    for (n = 0; n < tok.length; n++) { putc('^', stdout); }
    putc('\n', stdout);
}

void print_error(Error *err) {
    highlight_token(err->token);

    if (err->message) {
        printf("%s\n", err->message);
    }
}

void free_error(Error *err) {
    free(err->message);
    free(err);
}
