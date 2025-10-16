#include "tests.h"
#include "unity/unity.h"
#include "helpers.h"

#include "../src/parser.h"
#include "../src/code.h"
#include "unity/unity_internals.h"

#include <stdio.h>

void setUp(void) {}
void tearDown(void) {}

// compiler test
static void c_test(
    char *input,
    ExpectedConstants expectedConstants,
    Instructions *expectedInstructions
);

#define INT(n) TEST(int, n)
#define STR(s) TEST(str, s)
#define INS(...) TEST(ins, _I(__VA_ARGS__))

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
        (ExpectedConstants){},
        _I( make(OpTrue), make(OpPop) )
    );
    c_test(
        "false",
        (ExpectedConstants){},
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
        (ExpectedConstants){},
        _I(
            make(OpTrue),
            make(OpFalse),
            make(OpEqual),
            make(OpPop)
        )
    );
    c_test(
        "true != false",
        (ExpectedConstants){},
        _I(
            make(OpTrue),
            make(OpFalse),
            make(OpNotEqual),
            make(OpPop)
        )
    );
    c_test(
        "!true",
        (ExpectedConstants){},
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
        (ExpectedConstants){},
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

void test_hash_literals(void) {
    c_test(
        "{}",
        (ExpectedConstants){},
        _I(
            make(OpHash, 0),
            make(OpPop)
        )
    );
    c_test(
        "{1: 2, 3: 4, 5: 6}",
        _C( INT(1), INT(2), INT(3), INT(4), INT(5), INT(6) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpConstant, 2),
            make(OpConstant, 3),
            make(OpConstant, 4),
            make(OpConstant, 5),
            make(OpHash, 6),
            make(OpPop)
        )
    );
    c_test(
        "{1: 2 + 3, 4: 5 * 6}",
        _C( INT(1), INT(2), INT(3), INT(4), INT(5), INT(6) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpConstant, 2),
            make(OpAdd),
            make(OpConstant, 3),
            make(OpConstant, 4),
            make(OpConstant, 5),
            make(OpMul),
            make(OpHash, 4),
            make(OpPop)
        )
    );
}

void test_index_expressions(void) {
    c_test(
        "[1, 2, 3][1 + 1]",
        _C( INT(1), INT(2), INT(3), INT(1), INT(1) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpConstant, 2),
            make(OpArray, 3),
            make(OpConstant, 3),
            make(OpConstant, 4),
            make(OpAdd),
            make(OpIndex),
            make(OpPop)
        )
    );
    c_test(
        "{1: 2}[2 - 1]",
        _C( INT(1), INT(2), INT(2), INT(1) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpHash, 2),
            make(OpConstant, 2),
            make(OpConstant, 3),
            make(OpSub),
            make(OpIndex),
            make(OpPop)
        )
    );
}

void test_functions(void) {
    c_test(
        "fn() { return 5 + 10 }",
        _C(
            INT(5), INT(10),
            INS(
                make(OpConstant, 0),
                make(OpConstant, 1),
                make(OpAdd),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 2),
            make(OpPop)
        )
    );
    c_test(
        "fn() { 5 + 10 }",
        _C(
            INT(5), INT(10),
            INS(
                make(OpConstant, 0),
                make(OpConstant, 1),
                make(OpAdd),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 2),
            make(OpPop)
        )
    );
    c_test(
        "fn() { 1; 2 }",
        _C(
            INT(1), INT(2),
            INS(
                make(OpConstant, 0),
                make(OpPop),
                make(OpConstant, 1),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 2),
            make(OpPop)
        )
    );
    c_test(
        "fn() { }",
        _C( INS(make(OpReturn)) ),
        _I(
            make(OpConstant, 0),
            make(OpPop)
        )
    );
}

void test_compiler_scopes(void) {
    Compiler c;
    compiler_init(&c);

    if (c.scope_index != 0) {
        printf("scope_index wrong. got=%d, want=%d\n", c.scope_index, 0);
        TEST_FAIL();
    }

    SymbolTable *global_symbol_table = c.cur_symbol_table;

    emit(&c, OpMul);

    enter_scope(&c);
    if (c.scope_index != 1) {
        printf("scope_index wrong. got=%d, want=%d\n", c.scope_index, 1);
        TEST_FAIL();
    }

    emit(&c, OpSub);

    if (c.scopes.data[c.scope_index].instructions.length != 1) {
        printf("instructions length wrong. got=%d\n",
                c.scopes.data[c.scope_index].instructions.length);
        TEST_FAIL();
    }

    EmittedInstruction last = c.scopes.data[c.scope_index].last_instruction;
    if (last.opcode != OpSub) {
        printf("last_instruction Opcode wrong. got=%d, want=%d\n",
                last.opcode, OpSub);
        TEST_FAIL();
    }

    if (c.cur_symbol_table->outer != global_symbol_table) {
        TEST_FAIL_MESSAGE("compiler did not enclose symbol_table");
    }

    free(c.current_instructions->data); // free Instructions not added to
                                        // [c.constants]
    leave_scope(&c);
    if (c.scope_index != 0) {
        printf("scope_index wrong. got=%d, want=%d\n", c.scope_index, 0);
        TEST_FAIL();
    }

    if (c.cur_symbol_table != global_symbol_table) {
        TEST_FAIL_MESSAGE("compiler did not restore global_symbol_table");
    }

    if (c.cur_symbol_table->outer != NULL) {
        TEST_FAIL_MESSAGE("compiler modified global_symbol_table table incorrectly");
    }

    emit(&c, OpAdd);

    if (c.scopes.data[c.scope_index].instructions.length != 2) {
        printf("instructions length wrong. got=%d\n",
                c.scopes.data[c.scope_index].instructions.length);
        TEST_FAIL();
    }

    last = c.scopes.data[c.scope_index].last_instruction;
    if (last.opcode != OpAdd) {
        printf("last_instruction Opcode wrong. got=%d, want=%d\n",
                last.opcode, OpAdd);
        TEST_FAIL();
    }

    EmittedInstruction previous = c.scopes.data[c.scope_index].previous_instruction;
    if (previous.opcode != OpMul) {
        printf("previous_instruction Opcode wrong. got=%d, want=%d\n",
                previous.opcode, OpMul);
        TEST_FAIL();
    }

    compiler_free(&c);
}

void test_function_calls(void) {
    c_test(
        "fn() { 24 }()",
        _C(
            INT(24),
            INS(
                make(OpConstant, 0), // The literal "24"
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 1), // The compiled function
            make(OpCall, 0),
            make(OpPop)
        )
    );
    c_test(
        "let noArg = fn() { 24 };\
        noArg();\
        ",
        _C(
            INT(24),
            INS(
                make(OpConstant, 0), // The literal "24"
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 1), // The compiled function
            make(OpSetGlobal, 0),
            make(OpGetGlobal, 0),
            make(OpCall, 0),
            make(OpPop)
        )
    );
    c_test(
        "let noArg = fn(a) { a };\
        noArg(24);\
        ",
        _C(
            INS(
                make(OpGetLocal, 0),
                make(OpReturnValue)
            ),
            INT(24)
        ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpGetGlobal, 0),
            make(OpConstant, 1),
            make(OpCall, 1),
            make(OpPop)
        )
    );
    c_test(
        "let manyArg = fn(a, b, c) { a; b; c };\
        manyArg(24, 25, 26);\
        ",
        _C(
            INS(
                make(OpGetLocal, 0),
                make(OpPop),
                make(OpGetLocal, 1),
                make(OpPop),
                make(OpGetLocal, 2),
                make(OpReturnValue)
            ),
            INT(24),
            INT(25),
            INT(26)
        ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpGetGlobal, 0),
            make(OpConstant, 1),
            make(OpConstant, 2),
            make(OpConstant, 3),
            make(OpCall, 3),
            make(OpPop)
        )
    );
}

void test_let_statements_scopes(void) {
    c_test(
        "\
            let num = 55;\
            fn() { num }\
        ",
        _C(
            INT(55),
            INS(
                make(OpGetGlobal, 0),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpConstant, 1),
            make(OpPop)
        )
    );
    c_test(
        "\
            fn() {\
                let num = 55;\
                num\
            }\
        ",
        _C(
            INT(55),
            INS(
                make(OpConstant, 0),
                make(OpSetLocal, 0),
                make(OpGetLocal, 0),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 1),
            make(OpPop)
        )
    );
    c_test(
        "\
            fn() {\
                let a = 55;\
                let b = 77;\
                a + b\
            }\
        ",
        _C(
            INT(55), INT(77),
            INS(
                make(OpConstant, 0),
                make(OpSetLocal, 0),
                make(OpConstant, 1),
                make(OpSetLocal, 1),
                make(OpGetLocal, 0),
                make(OpGetLocal, 1),
                make(OpAdd),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 2),
            make(OpPop)
        )
    );
}

void test_builtins(void) {
    c_test(
        "\
            len([]);\
            push([], 1);\
        ",
        _C( INT(1) ),
        _I(
            make(OpGetBuiltin, 0),
            make(OpArray, 0),
            make(OpCall, 1),
            make(OpPop),
            make(OpGetBuiltin, 5),
            make(OpArray, 0),
            make(OpConstant, 0),
            make(OpCall, 2),
            make(OpPop)
        )
    );
    c_test(
        "fn() { len([]) }",
        _C(
            INS(
                make(OpGetBuiltin, 0),
                make(OpArray, 0),
                make(OpCall, 1),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpConstant, 0),
            make(OpPop)
        )
    );
}

static int test_instructions(Instructions *expected, Instructions *actual);
static int test_constants(ExpectedConstants expected, ConstantBuffer *actual);

static void
c_test(
    char *input,
    ExpectedConstants expectedConstants,
    Instructions *expectedInstructions
) {
    Program prog = test_parse(input);
    Compiler c;
    compiler_init(&c);

    error err = compile(&c, &prog);
    if (err != 0) {
        printf("compiler error: %s\n", err);
        free(err);
        TEST_FAIL();
    };
    Bytecode code = bytecode(&c);

    int _err = test_instructions(expectedInstructions, code.instructions);
    if (_err != 0) { TEST_FAIL(); }

    _err = test_constants(expectedConstants, code.constants);
    if (_err != 0) { TEST_FAIL(); }

    program_free(&prog);
    free(expectedInstructions->data);
    free(expectedInstructions);
    for (int i = 0; i < expectedConstants.length; i++) {
        if (expectedConstants.data[i].typ == test_ins) {
            Instructions *ins = expectedConstants.data[i].val._ins;
            free(ins->data);
            free(ins);
        }
    }
    free(expectedConstants.data);
    compiler_free(&c);
}

static int
test_instructions(Instructions *expected, Instructions *actual) {
    if (actual->length != expected->length) {
        printf("wrong instructions length.\nwant=\n");
        fprint_instructions(stdout, *expected);
        printf("got=\n");
        fprint_instructions(stdout, *actual);
        return -1;
    }

    for (int i = 0; i < expected->length; i++) {
        if (actual->data[i] != expected->data[i]) {
            printf("wrong instruction at %d. want=%d, got=%d\n",
                    i, expected->data[i], actual->data[i]);
            printf("want=\n");
            fprint_instructions(stdout, *expected);
            printf("got=\n");
            fprint_instructions(stdout, *actual);
            return -1;
        }
    }
    return 0;
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

    if (strcmp(actual.data.string->data, expected) != 0) {
        printf("object has wrong value. got='%s', want='%s'\n",
                actual.data.string->data, expected);
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

static int
test_constants(ExpectedConstants expected, ConstantBuffer *actual) {
    if (actual->length != expected.length) {
        printf("wrong constants length.\nwant=%d\ngot =%d\n",
                expected.length, actual->length);
        return -1;
    }

    int err;
    Test exp;
    Constant cur;
    for (int i = 0; i < expected.length; i++) {
        exp = expected.data[i];
        cur = actual->data[i];
        switch (exp.typ) {
            case test_int:
                err = test_integer_constant(exp.val._int, cur);
                if (err != 0) {
                    printf("constant %d - test_integer_object failed\n", i);
                    return -1;
                }
                break;

            case test_str:
                err = test_string_constant(exp.val._str, cur);
                if (err != 0) {
                    printf("constant %d - test_string_object failed\n", i);
                    return -1;
                }
                break;

            case test_ins:
                if (!expect_constant_is(c_Function, cur)) {
                    printf("constant %d - not a function\n", i);
                    return -1;
                }

                err = test_instructions(exp.val._ins,
                    &cur.data.function->instructions);
                if (err != 0) {
                    printf("constant %d - test_instructions failed\n", i);
                    return -1;
                }
                break;

            default:
                TEST_FAIL_MESSAGE("not implemented");
        }
    }
    return 0;
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_integer_arithmetic);
    RUN_TEST(test_boolean_expressions);
    RUN_TEST(test_conditionals);
    RUN_TEST(test_global_let_statements);
    RUN_TEST(test_string_expressions);
    RUN_TEST(test_array_literals);
    RUN_TEST(test_hash_literals);
    RUN_TEST(test_index_expressions);
    RUN_TEST(test_functions);
    RUN_TEST(test_compiler_scopes);
    RUN_TEST(test_function_calls);
    RUN_TEST(test_let_statements_scopes);
    RUN_TEST(test_builtins);
    return UNITY_END();
}
