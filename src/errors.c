#include "errors.h"
#include "token.h"
#include "utils.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

error error_num_args_(Token *tok, int expected, int actual) {
    return errorf("%.*s takes %d argument%s got %d",
                  tok->length, tok->start,
                  expected, expected == 1 ? "" : "s", actual);
}

error error_num_args(const char *name, int expected, int actual) {
    return errorf("%s takes %d argument%s got %d",
                  name, expected, expected == 1 ? "" : "s", actual);
}

error create_error(const char *message) {
    struct Error *err = calloc(1, sizeof(struct Error));
    if (err == NULL) { die("create error:"); }
    err->message = message;
    return err;
}

error verrorf(const char* format, va_list args) {
    char* msg = NULL;
    if (vasprintf(&msg, format, args) == -1) {
        die("verrorf vasprintf:");
    }
    return create_error(msg);
}

error errorf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    error err = verrorf(format, args);
    va_end(args);
    return err;
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

#define GET_DATA(tok) \
    int left, right, len; \
    const char *start = start_of_line(tok, &left); \
    end_of_line(tok, &right); \
    len = left + right;

static void left_pad(int length, FILE *s) {
    for (; length > 0; --length) { putc(' ', s); }
}

static void highlight(int length, FILE *s) {
    for (; length > 0; --length) { putc('^', s); }
}

void print_token(Token *tok, int leftpad, FILE *s) {
    GET_DATA(tok);

    left_pad(leftpad, s);
    fprintf(s, "%.*s\n", len, start);
}

void highlight_token(Token *tok, int leftpad, FILE *s) {
    GET_DATA(tok);

    left_pad(leftpad, s);
    fprintf(s, "%.*s\n", len, start);

    left_pad(leftpad + left, s);
    highlight(tok->length, s);
    putc('\n', stdout);
}

void print_error(error err, FILE *s) {
    if (err->token) {
        highlight_token(err->token, 2, s);
    }

    fprintf(s, "%s\n", err->message);
}

void free_error(error err) {
    if (err) {
        free((char *) err->message);
        free(err);
    }
}
