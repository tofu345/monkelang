#include "unity/unity.h"
#include "helpers.h"

#include <string.h>

void print_errors(ErrorBuffer* errs) {
    for (int i = 0; i < errs->length; i++) {
        printf("%s\n", errs->data[i]);
        free(errs->data[i]);
    }
    errs->length = 0;
}

void check_parser_errors(Parser* p) {
    if (p->errors.length == 0) { return; }
    printf("parser had %d errors\n", p->errors.length);
    print_errors(&p->errors);
    parser_free(p);
    TEST_FAIL();
}

Program test_parse(char *input) {
    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    check_parser_errors(&p);
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

ExpectedConstants
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
    return (ExpectedConstants){ buf, length };
}
