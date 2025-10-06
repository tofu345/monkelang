#include "unity/unity.h"
#include "helpers.h"

#include "../src/ast.h"
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

static void test_instructions(Instructions expected, Instructions actual);
static void test_constants(Constants expected, ConstantBuffer actual);

static void
c_test(
    char *input,
    Constants expectedConstants,
    Instructions expectedInstructions
) {
    Program prog = parse(input);
    Compiler c = {};
    compile(&c, &prog);
    check_compiler_errors(&c);
    Bytecode *code = bytecode(&c);

    test_instructions(expectedInstructions, code->instructions);
    test_constants(expectedConstants, code->constants);

    free(expectedInstructions.data);
    free(expectedConstants.data);
    program_destroy(&prog);
    compiler_destroy(&c);
}

static void
test_instructions(Instructions expected, Instructions actual) {
    if (actual.length != expected.length) {
        printf("wrong instructions length.\nwant=\n");
        fprint_instruction(stdout, expected);
        printf("got=\n");
        fprint_instruction(stdout, actual);
        TEST_FAIL();
    }

    for (int i = 0; i < expected.length; i++) {
        if (actual.data[i] != expected.data[i]) {
            printf("wrong instruction at %d. want=%d, got=%d\n",
                    i, expected.data[i], actual.data[i]);
            printf("want=\n");
            fprint_instruction(stdout, expected);
            printf("got=\n");
            fprint_instruction(stdout, actual);
            TEST_FAIL();
        }
    }
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
            case c_int:
                err = test_integer_object(expected.data[i].data._int, actual.data[i]);
                if (err != 0) {
                    printf("constant %d - testIntegerObject failed\n", i);
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
    return UNITY_END();
}
