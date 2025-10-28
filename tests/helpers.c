#include "unity/unity.h"
#include "helpers.h"

#include <string.h>

void print_parser_errors(Parser *p) {
    printf("parser had %d errors\n", p->errors.length);
    for (int i = 0; i < p->errors.length; i++) {
        Error err = p->errors.data[i];
        print_error(&err);
        free(err.message);
    }
    p->errors.length = 0;
}

Program test_parse(const char *input) {
    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    if (p.errors.length > 0) {
        printf("test %s\n", input);
        print_parser_errors(&p);
        parser_free(&p);
        TEST_FAIL();
    }
    parser_free(&p);
    return prog;
}

Instructions *
concat(Instructions cur, ...) {
    Instructions *concatted = malloc(sizeof(Instructions));
    if (concatted == NULL) { die("malloc"); }
    memcpy(concatted, &cur, sizeof(Instructions));

    va_list ap;
    int offset;
    va_start(ap, cur);
    while (1) {
        cur = va_arg(ap, Instructions);
        if (cur.data == NULL) break;

        offset = concatted->length;
        instructions_allocate(concatted, cur.length);
        memcpy(concatted->data + offset, cur.data, cur.length * sizeof(uint8_t));
        free(cur.data);
    }
    va_end(ap);
    return concatted;
}

Constants
constants(Test *t, ...) {
    va_list ap;
    int length = 0, capacity = 8;
    Test *buf = malloc(capacity * sizeof(Constant));
    if (buf == NULL) { die("malloc"); }

    va_start(ap, t);
    do {
        if (length >= capacity) {
            buf = realloc(buf, capacity *= 2);
            if (buf == NULL) die("realloc");
        }

        buf[length++] = *t;
        t = va_arg(ap, Test *);
    } while (t);
    va_end(ap);
    return (Constants){ buf, length };
}
