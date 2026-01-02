#include "unity/unity.h"
#include "helpers.h"

#include "../src/parser.h"
#include "../src/shared.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

Program parse_(const char *input) {
    Lexer l;
    lexer_init(&l, input, strlen(input));

    Program program = {0};
    ParseErrorBuffer errors = parse(parser(), &l, &program);
    if (errors.length > 0) {
        printf("test %s\n", input);
        printf("parser had %d errors\n", errors.length);
        print_parse_errors(&errors, stdout);
        program_free(&program);
        TEST_FAIL();
    }

    return program;
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

Tests
constants(Test *t, ...) {
    Tests buf = {0};
    int capacity = 0;

    va_list ap;
    va_start(ap, t);
    do {
        if (buf.length == capacity) {
            capacity = power_of_2_ceil(buf.length + 1);
            buf.data = realloc(buf.data, capacity * sizeof(Test));
            if (buf.data == NULL) die("realloc");
        }

        buf.data[buf.length++] = *t;
        t = va_arg(ap, Test *);
    } while (t);
    va_end(ap);

    return buf;
}
