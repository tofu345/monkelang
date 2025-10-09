#include "unity/unity.h"
#include "helpers.h"

#include "../src/parser.h"
#include "unity/unity_internals.h"

#include <stdio.h>

void setUp(void) {}
void tearDown(void) {}

// compiler test
static void c_test(
    char *input,
    Constants expectedConstants,
    Instructions expectedInstructions
);

void test_integer_arithmetic(void) {
    c_test(
        "1 + 2",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpAdd),
            make(OpPop)
        )
    );
    c_test(
        "1; 2;",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpPop),
            make(OpConstant, 1),
            make(OpPop)
        )
    );
    c_test(
        "1 - 2",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpSub),
            make(OpPop)
        )
    );
    c_test(
        "1 * 2",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpMul),
            make(OpPop)
        )
    );
    c_test(
        "2 / 1",
        _C( INT(2), INT(1) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpDiv),
            make(OpPop)
        )
    );
    c_test(
        "-1",
        _C( INT(1) ),
        _I(
            make(OpConstant, 0),
            make(OpMinus),
            make(OpPop)
        )
    );
}

void test_boolean_expressions(void) {
    c_test(
        "true",
        (Constants){},
        _I( make(OpTrue), make(OpPop) )
    );
    c_test(
        "false",
        (Constants){},
        _I( make(OpFalse), make(OpPop) )
    );
    c_test(
        "1 > 2",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpGreaterThan),
            make(OpPop)
        )
    );
    c_test(
        "1 < 2",
        _C( INT(2), INT(1) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpGreaterThan),
            make(OpPop)
        )
    );
    c_test(
        "1 == 2",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpEqual),
            make(OpPop)
        )
    );
    c_test(
        "1 != 2",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpNotEqual),
            make(OpPop)
        )
    );
    c_test(
        "true == false",
        (Constants){},
        _I(
            make(OpTrue),
            make(OpFalse),
            make(OpEqual),
            make(OpPop)
        )
    );
    c_test(
        "true != false",
        (Constants){},
        _I(
            make(OpTrue),
            make(OpFalse),
            make(OpNotEqual),
            make(OpPop)
        )
    );
    c_test(
        "!true",
        (Constants){},
        _I(
            make(OpTrue),
            make(OpBang),
            make(OpPop)
        )
    );
}

void test_conditionals(void) {
    c_test(
        "if (true) { 10 }; 3333;",
        _C( INT(10), INT(3333) ),
        _I(
            // 0000
            make(OpTrue),
            // 0001
            make(OpJumpNotTruthy, 10),
            // 0004
            make(OpConstant, 0),
            // 0007
            make(OpJump, 11),
            // 0010
            make(OpNull),
            // 0011
            make(OpPop),
            // 0012
            make(OpConstant, 1),
            // 0015
            make(OpPop)
        )
    );
    c_test(
        "if (true) { 10 } else { 20 }; 3333;",
        _C( INT(10), INT(20), INT(3333) ),
        _I(
            // 0000
            make(OpTrue),
            // 0001
            make(OpJumpNotTruthy, 10),
            // 0004
            make(OpConstant, 0),
            // 0007
            make(OpJump, 13),
            // 0010
            make(OpConstant, 1),
            // 0013
            make(OpPop),
            // 0014
            make(OpConstant, 2),
            // 0017
            make(OpPop)
        )
    );
}

void test_global_let_statements(void) {
    c_test(
        "\
        let one = 1;\
        let two = 2;\
        ",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpConstant, 1),
            make(OpSetGlobal, 1)
        )
    );
    c_test(
        "\
        let one = 1;\
        one;\
        ",
        _C( INT(1) ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpGetGlobal, 0),
            make(OpPop)
        )
    );
    c_test(
        "\
        let one = 1;\
        let two = one;\
        two;\
        ",
        _C( INT(1) ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpGetGlobal, 0),
            make(OpSetGlobal, 1),
            make(OpGetGlobal, 1),
            make(OpPop)
        )
    );
}

void test_string_expressions(void) {
    c_test(
        "\"monkey\"",
        _C( STR("monkey") ),
        _I(
            make(OpConstant, 0),
            make(OpPop)
        )
    );
    c_test(
        "\"mon\" + \"key\"",
        _C( STR("mon"), STR("key") ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpAdd),
            make(OpPop)
        )
    );
}

void test_array_literals(void) {
    c_test(
        "[]",
        (Constants){},
        _I(
            make(OpArray, 0),
            make(OpPop)
        )
    );
    c_test(
        "[1, 2, 3]",
        _C( INT(1), INT(2), INT(3) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpConstant, 2),
            make(OpArray, 3),
            make(OpPop)
        )
    );
    c_test(
        "[1 + 2, 3 - 4, 5 * 6]",
        _C( INT(1), INT(2), INT(3), INT(4), INT(5), INT(6) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpAdd),
            make(OpConstant, 2),
            make(OpConstant, 3),
            make(OpSub),
            make(OpConstant, 4),
            make(OpConstant, 5),
            make(OpMul),
            make(OpArray, 3),
            make(OpPop)
        )
    );
}

static void test_instructions(Instructions expected, Instructions actual);
static void test_constants(Constants expected, ConstantBuffer actual);

static void
c_test(
    char *input,
    Constants expectedConstants,
    Instructions expectedInstructions
) {
    Program prog = test_parse(input);
    Compiler c;
    compiler_init(&c);
    if (compile(&c, &prog) != 0) {
        printf("compiler had %d errors\n", c.errors.length);
        print_errors(&c.errors);
        compiler_free(&c);
        free(expectedInstructions.data);
        free(expectedConstants.data);
        program_free(&prog);
        TEST_FAIL();
    };
    Bytecode *code = bytecode(&c);

    test_instructions(expectedInstructions, code->instructions);
    test_constants(expectedConstants, code->constants);

    free(expectedInstructions.data);
    free(expectedConstants.data);
    program_free(&prog);
    compiler_free(&c);
}

static void
test_instructions(Instructions expected, Instructions actual) {
    if (actual.length != expected.length) {
        printf("wrong instructions length.\nwant=\n");
        fprint_instructions(stdout, expected);
        printf("got=\n");
        fprint_instructions(stdout, actual);
        TEST_FAIL();
    }

    for (int i = 0; i < expected.length; i++) {
        if (actual.data[i] != expected.data[i]) {
            printf("wrong instruction at %d. want=%d, got=%d\n",
                    i, expected.data[i], actual.data[i]);
            printf("want=\n");
            fprint_instructions(stdout, expected);
            printf("got=\n");
            fprint_instructions(stdout, actual);
            TEST_FAIL();
        }
    }
}

bool expect_constant_is(ConstantType expected, Constant actual) {
    if (actual.type != expected) {
        printf("constant is not %d. got=%d", expected, actual.type);
        return false;
    }
    return true;
}

static int
test_string_constant(char *expected, Constant actual) {
    if (!expect_constant_is(c_String, actual)) {
        return -1;
    }

    if (strcmp(actual.data.string, expected) != 0) {
        printf("object has wrong value. got='%s', want='%s'\n",
                actual.data.string, expected);
        return -1;
    }
    return 0;
}

static int
test_integer_constant(long expected, Constant actual) {
    if (!expect_constant_is(c_Integer, actual)) {
        return -1;
    }

    if (expected != actual.data.integer) {
        printf("object has wrong value. want=%ld got=%ld\n",
                expected, actual.data.integer);
        return -1;
    }
    return 0;
}

static void
test_constants(Constants expected, ConstantBuffer actual) {
    if (actual.length != expected.length) {
        printf("wrong constants length.\nwant=%d\ngot =%d\n",
                expected.length, actual.length);
        TEST_FAIL();
    }

    int err;
    for (int i = 0; i < expected.length; i++) {
        switch (expected.data[i].type) {
            case c_Integer:
                err = test_integer_constant(expected.data[i].data.integer, actual.data[i]);
                if (err != 0) {
                    printf("constant %d - test_integer_object failed\n", i);
                    TEST_FAIL();
                }
                break;

            case c_String:
                err = test_string_constant(expected.data[i].data.string, actual.data[i]);
                if (err != 0) {
                    printf("constant %d - test_string_object failed\n", i);
                    TEST_FAIL();
                }
                break;

            default:
                TEST_FAIL_MESSAGE("not implemented");
        }
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_integer_arithmetic);
    RUN_TEST(test_boolean_expressions);
    RUN_TEST(test_conditionals);
    RUN_TEST(test_global_let_statements);
    RUN_TEST(test_string_expressions);
    RUN_TEST(test_array_literals);
    return UNITY_END();
}
