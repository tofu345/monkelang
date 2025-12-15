#include "unity/unity.h"
#include "helpers.h"

#include "../src/parser.h"

#include <stdint.h>
#include <string.h>

Program test_parse(const char *input) {
    Parser p;
    parser_init(&p);

    Program prog = parse(&p, input);
    if (p.errors.length > 0) {
        printf("test %s\n", input);
        printf("parser had %d errors\n", p.errors.length);
        print_parser_errors(&p);

        program_free(&prog);
        parser_free(&p);
        TEST_FAIL();
    }

    parser_free(&p);
    return prog;
}

Instructions concat(Instructions first, ...) {
    Instructions concatted = first;
    uint8_t *data;
    int length, offset;

    va_list ap;
    va_start(ap, first);
    while (1) {
        offset = concatted.length;

        // read data and length of Instructions
        data = va_arg(ap, uint8_t *); // read first 8 bytes.
        if (data == NULL) break;
        length = va_arg(ap, int); // read remaining 8 bytes, ignoring capacity.

        instructions_allocate(&concatted, length);
        memcpy(concatted.data + offset, data, length);
        free(data);
    }
    va_end(ap);
    return concatted;
}

Constants
constants(Test *t, ...) {
    Constants c = {0};
    int capacity = 0;

    va_list ap;
    va_start(ap, t);
    do {
        if (c.length == capacity) {
            capacity = power_of_2_ceil(c.length + 1);
            c.data = realloc(c.data, capacity * sizeof(Test));
            if (c.data == NULL) die("realloc");
        }

        c.data[c.length++] = *t;
        t = va_arg(ap, Test *);
    } while (t);
    va_end(ap);

    return c;
}
