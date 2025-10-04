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
        uint8_t expected[4]; // NULL-terminated
    } tests[] = {
        {make(OpConstant, 65534), {OpConstant, 255, 254}},
        {make(OpAdd), {OpAdd}},
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
    Instructions tests[] = {
        make(OpAdd),
        make(OpConstant, 2),
        make(OpConstant, 65535)
    };
    int length = sizeof(tests) / sizeof(tests[0]);

    // concat all tests into tests[0]
    int index = tests[0].length;
    for (int i = 1; i < length; i++) {
        int len = tests[i].length;
        instructions_allocate(tests, len);
        memcpy(tests[0].data + index, tests[i].data, len * sizeof(uint8_t));
        index += len;
        free(tests[i].data);
    }

    char *expected_body = "0000 OpAdd\n\
0001 OpConstant 2\n\
0004 OpConstant 65535\n";

    char *buf = NULL;
    size_t len;
    FILE *fp = open_memstream(&buf, &len);

    int err = fprint_instruction(fp, tests[0]);
    TEST_ASSERT_MESSAGE(err == 0, "fprint_instruction failed");

    free(tests[0].data);
    fflush(fp);

    if (strcmp(expected_body, buf) != 0) {
        printf("\ninstructions wrongly formatted\n");
        printf("want='%s'\n", expected_body);
        printf("got ='%s'\n", buf);
        TEST_FAIL();
    }

    fclose(fp);
    free(buf);
}

#define _TEST(bytesRead, ...) \
    { bytesRead, {__VA_ARGS__}, make(__VA_ARGS__)}

void test_read_operands(void) {
    struct Test {
        int bytesRead;
        int bytecode[5]; // NULL-terminated
        Instructions actual;
    } tests[] = {
        _TEST(2, OpConstant, 65535),
    };
    int length = sizeof(tests) / sizeof(tests[0]);

    struct Test tt;
    int n;
    for (int i = 0; i < length; i++) {
        tt = tests[i];
        const Definition *def = lookup(tt.bytecode[0]);
        Operands operands = read_operands(&n, def, tt.actual.data);
        if (n != tt.bytesRead) {
            printf("n wrong. want=%d, got=%d\n",
                    tt.bytesRead, operands.length);
            TEST_FAIL();
        }

        bool fail = false;
        for (int i = 0; i < operands.length; i++) {
            //                       skip opcode -vvv
            if (operands.data[i] != tt.bytecode[i + 1]) {
                printf("operand %d wrong. want=%d, got=%d\n",
                        i, tt.bytecode[i + 1], operands.data[i]);
                fail = true;
            }
        }
        free(operands.data);
        free(tt.actual.data);
        if (fail) TEST_FAIL();
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_make);
    RUN_TEST(test_instructions_string);
    RUN_TEST(test_read_operands);
    return UNITY_END();
}
