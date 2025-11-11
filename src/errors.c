#include "errors.h"
#include "token.h"

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

// returns pointer to start of line of [token.start], skip spaces
// and set [distance].
const char *
start_of_line(Token *tok, int *distance) {
    const char *cur = tok->start;
    // [tok.position] is index of [cur] in source code.
    int pos = tok->position;
    // until start of source code or end of previous line.
    for (; pos > 0; --pos) {
        if (*(cur - 1) == '\n') {
            break;
        }
        cur--;
    }

    // skip spaces from start of line
    for (; *cur == ' ' && cur != tok->start;
            ++cur, ++pos) {}

    *distance = tok->position - pos;
    return cur;
}

// returns pointer to end of line of [token.start] and set [distance].
const char *
end_of_line(Token *tok, int *distance) {
    const char *cur = tok->start + tok->length;

    if (tok->type == t_Eof) {
        *distance = 0;
        return cur;
    }

    int dist = 0;
    while (true) {
        // walk right until end of source code or end of current line.
        char ch = *cur;
        if (ch == '\0' || ch == '\n') {
            break;
        }
        cur++;
        dist++;
    }
    *distance = dist + tok->length;
    return cur;
}

static void left_padding(int length) {
    for (; length > 0; --length) { putc(' ', stdout); }
}

static void highlight(int length) {
    for (; length > 0; --length) { putc('^', stdout); }
}

static const char *
_get_start_length(Token *tok, int *length) {
    int left_dist, right_dist;
    const char *start = start_of_line(tok, &left_dist);
    end_of_line(tok, &right_dist);
    *length = left_dist + right_dist;
    return start;
}

void print_token(Token *tok, int leftpad) {
    int length;
    const char *start = _get_start_length(tok, &length);
    left_padding(leftpad);
    printf("%.*s\n", length, start);
}

void print_token_line_number(Token *tok) {
    int length;
    const char *start = _get_start_length(tok, &length);
    printf("%4d | %.*s\n", tok->line, length, start);
}

void highlight_token(Token *tok, int leftpad) {
    int left_dist, right_dist;
    const char *start = start_of_line(tok, &left_dist);
    end_of_line(tok, &right_dist);

    left_padding(leftpad);
    printf("%.*s\n", left_dist + right_dist, start);

    left_padding(leftpad + left_dist);
    highlight(tok->length);
    putc('\n', stdout);
}

void highlight_token_with_line_number(Token *tok) {
    int left_dist, right_dist;
    const char *start = start_of_line(tok, &left_dist);
    end_of_line(tok, &right_dist);

    printf("%4d | %.*s\n", tok->line, left_dist + right_dist, start);
    left_padding(left_dist + 7); // + 'line | '
    highlight(tok->length);
    putc('\n', stdout);
}

void highlight_line_with_line_number(Token *tok) {
    int length;
    const char *start = _get_start_length(tok, &length);
    printf("%4d | %.*s\n", tok->line, length, start);
    left_padding(7); // 'line | '
    highlight(length);
    putc('\n', stdout);
}

void print_error(Error *err) {
    highlight_token_with_line_number(&err->token);

    if (err->message) {
        printf("%s\n", err->message);
    }
}

void free_error(Error *err) {
    if (err) {
        free(err->message);
        free(err);
    }
}
