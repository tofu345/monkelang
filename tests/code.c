#include "unity/unity.h"

#include "../src/code.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void setUp(void) {}

void tearDown(void) {}

void test_make(void) {
    struct Test {
        Instructions actual;
        uint8_t expected[5]; // NULL-terminated
    } tests[] = {
        {make(OpConstant, 65534), {OpConstant, 255, 254}},
        {make(OpAdd), {OpAdd}},
        {make(OpGetLocal, 255), {OpGetLocal, 255}},
        {make(OpClosure, 65534, 255), {OpClosure, 255, 254, 255}},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < tests_len; i++) {
        struct Test tt = tests[i];
        int j = 0;
        for (; tt.expected[j]; j++) {
            if (tt.actual.data[j] != tt.expected[j]) {
                printf("wrong byte at pos %d. want=%d, got=%d\n",
                        j, tt.expected[j], tt.actual.data[j]);
                TEST_FAIL();
            }
        }
        if (j != tt.actual.length) {
            printf("instruction %d has wrong length. want=%d, got=%d\n",
                    i, j, tt.actual.length);
            TEST_FAIL();
        }
        free(tt.actual.data);
    }
}

void test_instructions_string(void) {
    Instructions test = {0};
    make_into(&test, OpAdd);
    make_into(&test, OpGetLocal, 1);
    make_into(&test, OpConstant, 2);
    make_into(&test, OpConstant, 65535);
    make_into(&test, OpClosure, 65535, 255);

    char *expected_body = "\
0000 OpAdd\n\
0001 OpGetLocal 1\n\
0003 OpConstant 2\n\
0006 OpConstant 65535\n\
0009 OpClosure 65535 255\n";

    char *buf = NULL;
    size_t len;
    FILE *fp = open_memstream(&buf, &len);
    TEST_ASSERT_NOT_NULL_MESSAGE(fp, "open_memstream fail");

    int err = fprint_instructions(fp, &test);
    TEST_ASSERT_MESSAGE(err == 0, "fprint_instructions failed");
    free(test.data);

    fclose(fp);
    if (strcmp(expected_body, buf) != 0) {
        printf("instructions wrongly formatted\n");
        printf("want=\n%s\n", expected_body);
        printf("got=\n%s\n", buf);
        TEST_FAIL();
    }

    free(buf);
}

#define _TEST(bytesRead, ...) \
    { bytesRead, {__VA_ARGS__}, make(__VA_ARGS__)}

void test_read_operands(void) {
    struct Test {
        int n_bytes;
        int bytecode[5]; // NULL-terminated
        Instructions actual;
    } tests[] = {
        _TEST(2, OpConstant, 65535),
        _TEST(3, OpClosure, 65535, 255),
    };
    int length = sizeof(tests) / sizeof(tests[0]);

    bool fail = false;
    int n;
    for (int i = 0; i < length; i++) {
        struct Test tt = tests[i];
        const Definition *def = lookup(tt.bytecode[0]);
        Operands operands = read_operands(&n, def, tt.actual.data);
        if (n != tt.n_bytes) {
            printf("n_bytes wrong. want=%d, got=%d\n",
                    tt.n_bytes, operands.length);
            fail = true;
        }

        for (int i = 0; i < operands.length; i++) {
            //                       skip opcode -vvvvv
            if (operands.widths[i] != tt.bytecode[i + 1]) {
                printf("operand %d wrong. want=%d, got=%d\n",
                        i, tt.bytecode[i + 1], operands.widths[i]);
                fail = true;
                break;
            }
        }

        free(operands.widths);
        free(tt.actual.data);
    }
    if (fail) { TEST_FAIL(); }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_make);
    RUN_TEST(test_instructions_string);
    RUN_TEST(test_read_operands);
    return UNITY_END();
}
