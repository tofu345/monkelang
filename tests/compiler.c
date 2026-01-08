#include "unity/unity.h"
#include "helpers.h"

#include "../src/code.h"

#include <stdio.h>

Compiler c;
Program prog;
error err;

// initialize Program and Compiler.
static void init(void);

static void c_test(char *input, Tests expectedConstants,
                   Instructions expectedInstructions);
static void c_test_error(const char *input, const char *expected_error);

#define INT(n) TEST(int, n)
#define FLOAT(n) TEST(float, n)
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
        "0; 0.;",
        _C( INT(0), FLOAT(0) ),
        _I(
            make(OpConstant, 0),
            make(OpPop),
            make(OpConstant, 1),
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
        NO_CONSTANTS,
        _I( make(OpTrue), make(OpPop) )
    );
    c_test(
        "false",
        NO_CONSTANTS,
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
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpLessThan),
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
        NO_CONSTANTS,
        _I(
            make(OpTrue),
            make(OpFalse),
            make(OpEqual),
            make(OpPop)
        )
    );
    c_test(
        "true != false",
        NO_CONSTANTS,
        _I(
            make(OpTrue),
            make(OpFalse),
            make(OpNotEqual),
            make(OpPop)
        )
    );
    c_test(
        "!true",
        NO_CONSTANTS,
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
            // condition
            make(OpTrue),
            make(OpJumpNotTruthy, 10),

            // consequence
            make(OpConstant, 0),
            // skip alternative
            make(OpJump, 11),

            // alternative
            make(OpNothing),

            // end
            make(OpPop),

            // 3333;
            make(OpConstant, 1),
            make(OpPop)
        )
    );
    c_test(
        "if (true) { 10 } else { 20 }; 3333;",
        _C( INT(10), INT(20), INT(3333) ),
        _I(
            // condition
            make(OpTrue),
            make(OpJumpNotTruthy, 10),

            // consequence
            make(OpConstant, 0),
            // skip alternative
            make(OpJump, 13),

            // alternative
            make(OpConstant, 1),

            // end
            make(OpPop),

            // 3333;
            make(OpConstant, 2),
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
    c_test(
        "\
        let one = 1, two = one;\
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

    c_test_error("let b = b", "undefined variable 'b'");
    c_test_error("b", "undefined variable 'b'");
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
    c_test(
        "\"monkey\" + \"monkey\"",
        _C( STR("monkey") ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 0),
            make(OpAdd),
            make(OpPop)
        )
    );
}

void test_array_literals(void) {
    c_test(
        "[]",
        NO_CONSTANTS,
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

void test_table_literals(void) {
    c_test(
        "{}",
        NO_CONSTANTS,
        _I(
            make(OpTable, 0),
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
            make(OpTable, 6),
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
            make(OpTable, 4),
            make(OpPop)
        )
    );
    c_test(
        "\
        let one = 1, two = 2, three = 3;\
        {one, two, three, \"four\": 4}",
        _C( INT(1), INT(2), INT(3),
            STR("one"), STR("two"), STR("three"),
            STR("four"), INT(4) ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpConstant, 1),
            make(OpSetGlobal, 1),
            make(OpConstant, 2),
            make(OpSetGlobal, 2),

            make(OpConstant, 3),
            make(OpGetGlobal, 0),
            make(OpConstant, 4),
            make(OpGetGlobal, 1),
            make(OpConstant, 5),
            make(OpGetGlobal, 2),
            make(OpConstant, 6),
            make(OpConstant, 7),
            make(OpTable, 8),
            make(OpPop)
        )
    );
}

void test_index_expressions(void) {
    c_test(
        "[1, 2, 3][1 + 1]",
        _C( INT(1), INT(2), INT(3) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpConstant, 2),
            make(OpArray, 3),
            make(OpConstant, 0),
            make(OpConstant, 0),
            make(OpAdd),
            make(OpIndex),
            make(OpPop)
        )
    );
    c_test(
        "{1: 2}[2 - 1]",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpTable, 2),
            make(OpConstant, 1),
            make(OpConstant, 0),
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
            make(OpClosure, 2, 0),
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
            make(OpClosure, 2, 0),
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
            make(OpClosure, 2, 0),
            make(OpPop)
        )
    );
}

void test_functions_without_return_value(void) {
    c_test(
        "fn() { }",
        _C( INS(make(OpReturn)) ),
        _I(
            make(OpClosure, 0, 0),
            make(OpPop)
        )
    );
}

void test_compiler_scopes(void) {
    init();

    enter_scope(&c);

    if (c.scopes.length != 1) {
        printf("scope_index wrong. got=%d, want=%d\n", c.scopes.length, 1);
        TEST_FAIL();
    }

    SymbolTable *global_symbol_table = c.cur_symbol_table;

    emit(&c, OpMul);

    enter_scope(&c);
    if (c.scopes.length != 2) {
        printf("scope_index wrong. got=%d, want=%d\n", c.scopes.length, 2);
        TEST_FAIL();
    }

    emit(&c, OpSub);

    if (c.cur_instructions->length != 1) {
        printf("instructions length wrong. got=%d\n",
                c.cur_instructions->length);
        TEST_FAIL();
    }

    EmittedInstruction last = c.cur_scope->last_instruction;
    if (last.opcode != OpSub) {
        printf("last_instruction Opcode wrong. got=%d, want=%d\n",
                last.opcode, OpSub);
        TEST_FAIL();
    }

    if (c.cur_symbol_table->outer != global_symbol_table) {
        TEST_FAIL_MESSAGE("compiler did not enclose symbol_table");
    }

    free_function(c.cur_scope->function);
    SymbolTable *prev = c.cur_symbol_table;
    leave_scope(&c);
    symbol_table_free(prev);

    if (c.scopes.length != 1) {
        printf("scope_index wrong. got=%d, want=%d\n", c.scopes.length, 1);
        TEST_FAIL();
    }

    if (c.cur_symbol_table != global_symbol_table) {
        TEST_FAIL_MESSAGE("compiler did not restore global_symbol_table");
    }

    if (c.cur_symbol_table->outer != NULL) {
        TEST_FAIL_MESSAGE("compiler modified global_symbol_table table incorrectly");
    }

    emit(&c, OpAdd);

    if (c.cur_instructions->length != 2) {
        printf("instructions length wrong. got=%d\n",
                c.cur_instructions->length);
        TEST_FAIL();
    }

    last = c.cur_scope->last_instruction;
    if (last.opcode != OpAdd) {
        printf("last_instruction Opcode wrong. got=%d, want=%d\n",
                last.opcode, OpAdd);
        TEST_FAIL();
    }

    EmittedInstruction previous = c.cur_scope->previous_instruction;
    if (previous.opcode != OpMul) {
        printf("previous_instruction Opcode wrong. got=%d, want=%d\n",
                previous.opcode, OpMul);
        TEST_FAIL();
    }
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
            make(OpClosure, 1, 0),
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
            make(OpClosure, 1, 0),
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
            make(OpClosure, 0, 0),
            make(OpSetGlobal, 0),
            make(OpConstant, 1),
            make(OpGetGlobal, 0),
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
            make(OpClosure, 0, 0),
            make(OpSetGlobal, 0),
            make(OpConstant, 1),
            make(OpConstant, 2),
            make(OpConstant, 3),
            make(OpGetGlobal, 0),
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
            make(OpClosure, 1, 0),
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
            make(OpClosure, 1, 0),
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
            make(OpClosure, 2, 0),
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
            make(OpArray, 0),
            make(OpGetBuiltin, 0),
            make(OpCall, 1),
            make(OpPop),
            make(OpArray, 0),
            make(OpConstant, 0),
            make(OpGetBuiltin, 5),
            make(OpCall, 2),
            make(OpPop)
        )
    );
    c_test(
        "fn() { len([]) }",
        _C(
            INS(
                make(OpArray, 0),
                make(OpGetBuiltin, 0),
                make(OpCall, 1),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpClosure, 0, 0),
            make(OpPop)
        )
    );
}

void test_closures(void) {
    c_test(
        "\
        fn(a) {\
            fn(b) {\
                a + b\
            }\
        }\
        ",
        _C(
            INS(
                make(OpGetFree, 0),
                make(OpGetLocal, 0),
                make(OpAdd),
                make(OpReturnValue)
            ),
            INS(
                make(OpGetLocal, 0),
                make(OpClosure, 0, 1),
                make(OpReturnValue)
           )
        ),
        _I(
            make(OpClosure, 1, 0),
            make(OpPop)
        )
    );
    c_test(
        "\
        fn(a) {\
            fn(b) {\
                fn(c) {\
                    a + b + c\
                }\
            }\
        };\
        ",
        _C(
            INS(
                make(OpGetFree, 0),
                make(OpGetFree, 1),
                make(OpAdd),
                make(OpGetLocal, 0),
                make(OpAdd),
                make(OpReturnValue)
            ),
            INS(
                make(OpGetFree, 0),
                make(OpGetLocal, 0),
                make(OpClosure, 0, 2),
                make(OpReturnValue)
           ),
           INS(
                make(OpGetLocal, 0),
                make(OpClosure, 1, 1),
                make(OpReturnValue)
           )
        ),
        _I(
            make(OpClosure, 2, 0),
            make(OpPop)
        )
    );
    c_test(
        "\
        let global = 55;\
\
        fn() {\
            let a = 66;\
\
            fn() {\
                let b = 77;\
                fn() {\
                    b = 88;\
                    let c = 99;\
                    global + a + b + c;\
                }\
            }\
        }\
        ",
        _C(
            INT(55), INT(66), INT(77), INT(88), INT(99),
            INS(
                make(OpConstant, 3),
                make(OpSetFree, 0),
                make(OpConstant, 4),
                make(OpSetLocal, 0),
                make(OpGetGlobal, 0),
                make(OpGetFree, 1),
                make(OpAdd),
                make(OpGetFree, 0),
                make(OpAdd),
                make(OpGetLocal, 0),
                make(OpAdd),
                make(OpReturnValue)
            ),
            INS(
                make(OpConstant, 2),
                make(OpSetLocal, 0),
                make(OpGetLocal, 0),
                make(OpGetFree, 0),
                make(OpClosure, 5, 2),
                make(OpReturnValue)
           ),
           INS(
                make(OpConstant, 1),
                make(OpSetLocal, 0),
                make(OpGetLocal, 0),
                make(OpClosure, 6, 1),
                make(OpReturnValue)
           )
        ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpClosure, 7, 0),
            make(OpPop)
        )
    );
}

void test_recursive_functions(void) {
    c_test(
        "\
            let countDown = fn(x) { countDown(x - 1); };\
            countDown(1);\
        ",
        _C(
            INT(1),
            INS(
                make(OpGetLocal, 0),
                make(OpConstant, 0),
                make(OpSub),
                make(OpCurrentClosure),
                make(OpCall, 1),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpClosure, 1, 0),
            make(OpSetGlobal, 0),
            make(OpConstant, 0),
            make(OpGetGlobal, 0),
            make(OpCall, 1),
            make(OpPop)
        )
    );
    c_test(
        "\
            let wrapper = fn() {\
                let countDown = fn(x) { countDown(x - 1); };\
                countDown(1);\
            };\
            wrapper();\
        ",
        _C(
            INT(1),
            INS(
                make(OpGetLocal, 0),
                make(OpConstant, 0),
                make(OpSub),
                make(OpCurrentClosure),
                make(OpCall, 1),
                make(OpReturnValue)
            ),
            INS(
                make(OpClosure, 1, 0),
                make(OpSetLocal, 0),
                make(OpConstant, 0),
                make(OpGetLocal, 0),
                make(OpCall, 1),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpClosure, 2, 0),
            make(OpSetGlobal, 0),
            make(OpGetGlobal, 0),
            make(OpCall, 0),
            make(OpPop)
        )
    );
}

void test_assignments(void) {
    c_test(
        "let foobar = 1; foobar = 0;",
        _C( INT(1), INT(0) ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),
            make(OpConstant, 1),
            make(OpSetGlobal, 0)
        )
    );
    c_test(
        "let foobar = [0, 1, 2]; foobar[0] = 5;",
        _C( INT(0), INT(1), INT(2), INT(5) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpConstant, 2),
            make(OpArray, 3),
            make(OpSetGlobal, 0),

            make(OpConstant, 3),  // 5
            make(OpGetGlobal, 0), // foobar
            make(OpConstant, 0),  // 0
            make(OpSetIndex)
        )
    );
    c_test(
        "let foobar = {0: 1}; foobar[0] = 5;",
        _C( INT(0), INT(1), INT(5) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpTable, 2),
            make(OpSetGlobal, 0),

            make(OpConstant, 2),
            make(OpGetGlobal, 0),
            make(OpConstant, 0),
            make(OpSetIndex)
        )
    );
    c_test(
        "let a = nothing; a = 5;",
        _C( INT(5) ),
        _I(
            make(OpNothing, 0),
            make(OpSetGlobal, 0),
            make(OpConstant, 0),
            make(OpSetGlobal, 0)
        )
    );
    c_test(
        "\
            let func = fn() {\
                let a = 25;\
                a = a * 2;\
                return a;\
            };\
        ",
        _C(
            INT(25), INT(2),
            INS(
                make(OpConstant, 0),
                make(OpSetLocal, 0),
                make(OpGetLocal, 0),
                make(OpConstant, 1),
                make(OpMul),
                make(OpSetLocal, 0),
                make(OpGetLocal, 0),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpClosure, 2, 0),
            make(OpSetGlobal, 0)
        )
    );

    c_test_error("b = 1", "undefined variable 'b' is not assignable");
    c_test_error(
        "let a = fn() { a = 1; }",
        "function 'a' is not assignable"
    );
    c_test_error(
        "len = 1;",
        "builtin len() is not assignable"
    );
}

void test_operator_assignments(void) {
    c_test(
        "\
        let var = 1;\
        var += 3;\
        ",
        _C( INT(1), INT(3) ),
        _I(
            make(OpConstant, 0),
            make(OpSetGlobal, 0),

            make(OpGetGlobal, 0),
            make(OpConstant, 1),
            make(OpAdd),
            make(OpSetGlobal, 0)
        )
    );
    c_test(
        "\
        let var = [1];\
        var[0] += 2;\
        ",
        _C( INT(1), INT(0), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpArray, 1),
            make(OpSetGlobal, 0),

            make(OpGetGlobal, 0),
            make(OpConstant, 1),
            make(OpIndex),
            make(OpConstant, 2),
            make(OpAdd),
            make(OpGetGlobal, 0),
            make(OpConstant, 1),
            make(OpSetIndex)
        )
    );
    c_test(
        "\
        let var = {1: 2};\
        var[1] += 2;\
        ",
        _C( INT(1), INT(2) ),
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1),
            make(OpTable, 2),
            make(OpSetGlobal, 0),

            make(OpGetGlobal, 0),
            make(OpConstant, 0),
            make(OpIndex),
            make(OpConstant, 1),
            make(OpAdd),
            make(OpGetGlobal, 0),
            make(OpConstant, 0),
            make(OpSetIndex)
        )
    );
}

void test_return_statements(void) {
    c_test(
        "fn() { if (true) { return; } else { 20 }; 3333; }",
        _C(
            INT(20),
            INT(3333),
            INS(
                make(OpTrue),
                make(OpJumpNotTruthy, 9),
                make(OpReturn),
                make(OpNothing),
                make(OpJump, 12),
                make(OpConstant, 0),
                make(OpPop),
                make(OpConstant, 1),
                make(OpReturnValue)
            )
        ),
        _I(
            make(OpClosure, 2, 0),
            make(OpPop)
        )
    );

    c_test_error("return;", "return statement outside function");
}

void test_loops(void) {
    c_test(
        "\
        for (let i = 0; i < 5; i += 1) {\
            puts(\"i:\", i);\
        }\
        ",
        _C( INT(0), INT(5), STR("i:"), INT(1) ),
        _I(
            // start
            make(OpConstant, 0),    // let i = 0;
            make(OpSetGlobal, 0),

            // condition
            make(OpGetGlobal, 0),   // i < 5;
            make(OpConstant, 1),
            make(OpLessThan),

            make(OpJumpNotTruthy, 40), // to after loop

            // body
            make(OpConstant, 2),    // puts(...)
            make(OpGetGlobal, 0),
            make(OpGetBuiltin, 1),
            make(OpCall, 2),
            make(OpPop),

            // update
            make(OpGetGlobal, 0),   // i += 1;
            make(OpConstant, 3),
            make(OpAdd),
            make(OpSetGlobal, 0),

            make(OpJump, 6) // to condition
        )
    );
    c_test(
        "\
        for (;;) {\
            puts(\"i\");\
        }\
        ",
        _C( STR("i") ),
        _I(
            // start

            // condition
            make(OpTrue),

            make(OpJumpNotTruthy, 15), // to after loop

            // body
            make(OpConstant, 0),
            make(OpGetBuiltin, 1),
            make(OpCall, 1),
            make(OpPop),

            // update

            make(OpJump, 0) // to condition
        )
    );
    c_test(
        "\
        while (true) {\
            puts(\"i\");\
        }\
        ",
        _C( STR("i") ),
        _I(
            // condition
            make(OpTrue),

            make(OpJumpNotTruthy, 15), // to after loop

            // body
            make(OpConstant, 0),
            make(OpGetBuiltin, 1),
            make(OpCall, 1),
            make(OpPop),

            make(OpJump, 0) // to condition
        )
    );

    c_test(
        "\
        for (let i = 0 ;; i += 1) {\
            if (i > 5) {\
                break\
            } else {\
                continue\
            }\
        }\
        ",
        _C( INT(0), INT(5), INT(1) ),
        _I(
            // start
            make(OpConstant, 0), // let i = 0;
            make(OpSetGlobal, 0),

            // condition
            make(OpTrue),

            make(OpJumpNotTruthy, 45), // to after loop

            // body
            // if statement
            make(OpGetGlobal, 0), // i > 5
            make(OpConstant, 1),
            make(OpGreaterThan),
            make(OpJumpNotTruthy, 27),

            // if statement consequence
            make(OpJump, 45), // break
            make(OpNothing),
            make(OpJump, 31),

            // if statement alternative
            make(OpJump, 32), // continue
            make(OpNothing),
            make(OpPop),

            // update
            make(OpGetGlobal, 0), // i += 1;
            make(OpConstant, 2),
            make(OpAdd),
            make(OpSetGlobal, 0),

            make(OpJump, 6) // to condition
        )
    );
    c_test(
        "\
        for (let i = 0 ;; i += 1) {\
            if (i > 5) {\
                for (;;) {\
                    break\
                };\
                break\
            }\
        }\
        ",
        _C( INT(0), INT(5), INT(1) ),
        _I(
            // outer loop start
            make(OpConstant, 0), // let i = 0;
            make(OpSetGlobal, 0),

            // outer loop condition
            make(OpTrue),
            make(OpJumpNotTruthy, 52), // to after outer loop

            // outer loop body
            // if statement
            make(OpGetGlobal, 0), // i > 5
            make(OpConstant, 1),
            make(OpGreaterThan),
            make(OpJumpNotTruthy, 37),

            // if statement consequence
            // inner loop start

            // inner loop condition
            make(OpTrue),
            make(OpJumpNotTruthy, 30), // to after inner loop

            // inner loop body
            make(OpJump, 30), // break inner loop
            make(OpJump, 20), // to inner loop condition

            // inner loop update
            // inner loop end

            make(OpJump, 52), // break outer loop
            make(OpNothing),
            make(OpJump, 38), // if statement alternative
            make(OpNothing),
            make(OpPop),

            // outer loop update
            make(OpGetGlobal, 0), // i += 1;
            make(OpConstant, 2),
            make(OpAdd),
            make(OpSetGlobal, 0),

            make(OpJump, 6) // to outer loop condition
            // outer loop end
        )
    );

    c_test_error("fn() { break; }", "cannot have 'break' outside of loop");
    c_test_error("break;", "cannot have 'break' outside of loop");
    c_test_error("continue;", "cannot have 'continue' outside of loop");

    // this is intended behaviour
    c_test_error("\
        for (;;) {\
            fn() { break; }()\
        }",
        "cannot have 'break' outside of loop"
    );
    c_test_error("\
        while (true) {\
            fn() { continue; }()\
        }",
        "cannot have 'continue' outside of loop"
    );
}

void test_source_mappings(void) {
    char *input = "\
    let a = 0;\n\
    a += 2;\n\
    1 + 2 * a;\n\
    let func = fn(a) { a + 24 };\n\
    puts(type(func), \"func(10):\", func(10));\n\
    a = !true == !(false == true);\n\
    for (let i = 0; i < 5; i += 1) {}\n\
    ";

    SourceMapping exp_mappings[] = {
        { 0, NODE(n_Identifier, NULL) },            // let a = 0;
        { 12, NODE(n_OperatorAssignment, NULL) },   // a += 2;
        { 13, NODE(n_OperatorAssignment, NULL) },

        { 16, NODE(n_ExpressionStatement, NULL) },  // 1 + 2 * a;
        { 25, NODE(n_InfixExpression, NULL) },      // 1 + 2 * a;
        //                                                   ^
        { 26, NODE(n_InfixExpression, NULL) },      // 1 + 2 * a;
        //                                               ^

        { 28, NODE(n_Identifier, NULL) },           // let func = fn(a) { a + 24 };
        { 35, NODE(n_ExpressionStatement, NULL) },  // puts(type(func), "func(10):", func(10));
        { 40, NODE(n_CallExpression, NULL) },       // puts(type(func), "func(10):", func(10));
        //                                                  ^^^^
        { 51, NODE(n_CallExpression, NULL) },       // puts(type(func), "func(10):", func(10));
        //                                                                           ^^^^
        { 55, NODE(n_CallExpression, NULL) },       // puts(type(func), "func(10):", func(10));
        //                                             ^^^^

        { 59, NODE(n_PrefixExpression, NULL) },     // a = !true == !(false == true);
        //                                                 ^
        { 62, NODE(n_InfixExpression, NULL) },      // a = !true == !(false == true);
        //                                                                  ^^
        { 63, NODE(n_PrefixExpression, NULL) },     // a = !true == !(false == true);
        //                                                          ^
        { 64, NODE(n_InfixExpression, NULL) },      // a = !true == !(false == true);
        //                                                       ^^
        { 65, NODE(n_Assignment, NULL) },           // a = !true == !(false == true);
        //                                               ^

        { 68, NODE(n_LoopStatement, NULL) },        // for (let i = 0; i < 5; i += 1) {}
        { 68, NODE(n_Identifier, NULL) },           // for (let i = 0; i < 5; i += 1) {}
        //                                                  ^^^
        { 80, NODE(n_InfixExpression, NULL) },      // for (let i = 0; i < 5; i += 1) {}
        //                                                               ^
        { 90, NODE(n_OperatorAssignment, NULL) },   // for (let i = 0; i < 5; i += 1) {}
        //                                                                      ^^
        { 91, NODE(n_OperatorAssignment, NULL) },
    };
    int len = sizeof(exp_mappings) / sizeof(exp_mappings[0]);

    init();

    prog = parse_(input);

    err = compile(&c, &prog, 0);
    if (err != 0) {
        print_error(err, stdout);
        TEST_FAIL();
    };

    Bytecode code = bytecode(&c);
    CompiledFunction *main_fn = code.main_function;

    // fprint_instructions_mappings(
    //         stdout, main_fn->mappings, main_fn->instructions);

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            len, main_fn->mappings.length, "wrong mappings length");

    SourceMapping *mappings = main_fn->mappings.data;
    for (int i = 0; i < len; ++i) {
        if (exp_mappings[i].position != mappings[i].position) {
            printf("SourceMapping with type %d at:\n", mappings[i].node.typ);
            highlight_token(node_token(mappings[i].node), 0, stdout);
            printf("has wrong position, expected %d, got %d\n",
                    exp_mappings[i].position, mappings[i].position);
            TEST_FAIL();
        }

        if (exp_mappings[i].node.typ != mappings[i].node.typ) {
            printf("SourceMapping at:\n");
            highlight_token(node_token(mappings[i].node), 0, stdout);
            printf("has wrong node type, expected %d, got %d\n",
                    exp_mappings[i].node.typ, mappings[i].node.typ);
            TEST_FAIL();
        }
    }
}

void test_modules(void) {
    c_test_error("require()", "require() expects 1 argument of type string got 0");
    c_test_error(
        "require(\"a.monke\", \"b.monke\")",
        "require() expects 1 argument of type string got 2"
    );
}

static int test_instructions(Instructions expected, Instructions actual);
static int test_constants(Tests expected, ConstantBuffer actual);

// Because each test contains multiple c_test() and c_test_error() which must
// reinitialize before running.
bool initialized = false;

static void
init(void) {
    compiler_init(&c);
    prog = (Program){0};
    err = NULL;

    initialized = true;
}

static void
cleanup(void) {
    program_free(&prog);
    free_error(err);
    compiler_free(&c);

    initialized = false;
}

void setUp(void) {}

void tearDown(void) {
    if (initialized) {
        cleanup();
    }
}

static void
c_test_error(const char *input, const char *expected_error) {
    init();

    prog = parse_(input);

    err = compile(&c, &prog, 0);
    if (!err) {
        printf("expected compiler error but received none\n");
        printf("in test: '%s'\n\n", input);
        TEST_FAIL();
    }

    if (strcmp(err->message, expected_error) != 0) {
        printf("wrong compiler error\nwant= '%s'\ngot = '%s'\n",
                expected_error, err->message);
        printf("in test: '%s'\n\n", input);
        TEST_FAIL();
    }

    cleanup();
}

static void c_test(char *input, Tests expectedConstants,
                   Instructions expectedInstructions) {
    init();

    prog = parse_(input);

    err = compile(&c, &prog, 0);
    if (err != 0) {
        print_error(err, stdout);
        TEST_FAIL();
    };

    Bytecode code = bytecode(&c);

    int err1 = test_instructions(expectedInstructions,
                code.main_function->instructions);

    int err2 = test_constants(expectedConstants, c.constants);

    free(expectedInstructions.data);
    for (int i = 0; i < expectedConstants.length; i++) {
        if (expectedConstants.data[i].typ == test_ins) {
            free(expectedConstants.data[i].val._ins.data);
        }
    }
    free(expectedConstants.data);

    if (err1 || err2) {
        printf("in test: '%s'\n\n", input);
        TEST_FAIL();
    }

    cleanup();
}

static int
test_instructions(Instructions expected, Instructions actual) {
    if (actual.length != expected.length) {
        printf("wrong instructions length.\nwant=\n");
        fprint_instructions(stdout, &expected);
        printf("got=\n");
        fprint_instructions(stdout, &actual);
        return -1;
    }

    for (int i = 0; i < expected.length; i++) {
        if (actual.data[i] != expected.data[i]) {
            printf("wrong instruction at %d. want=%d, got=%d\n",
                    i, expected.data[i], actual.data[i]);
            printf("want=\n");
            fprint_instructions(stdout, &expected);
            printf("got=\n");
            fprint_instructions(stdout, &actual);
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

    Token *tok = actual.data.string;
    if (strncmp(tok->start, expected, tok->length) != 0) {
        printf("object has wrong value. got='%.*s', want='%s'\n",
                tok->length, tok->start, expected);
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
test_float_constant(double expected, Constant actual) {
    if (!expect_constant_is(c_Float, actual)) {
        return -1;
    }

    if (expected != actual.data.floating) {
        printf("object has wrong value. want=%f got=%f\n",
                expected, actual.data.floating);
        return -1;
    }
    return 0;
}

static int
test_constants(Tests expected, ConstantBuffer actual) {
    if (actual.length != expected.length) {
        printf("wrong constants length.\nwant=%d\ngot =%d\n",
                expected.length, actual.length);
        return -1;
    }

    int err;
    Test exp;
    Constant cur;
    for (int i = 0; i < expected.length; i++) {
        exp = expected.data[i];
        cur = actual.data[i];
        switch (exp.typ) {
            case test_int:
                err = test_integer_constant(exp.val._int, cur);
                if (err != 0) {
                    printf("constant %d - test_integer_object failed\n", i);
                    return -1;
                }
                break;

            case test_float:
                err = test_float_constant(exp.val._float, cur);
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

                CompiledFunction *func = cur.data.function;
                err = test_instructions(exp.val._ins, func->instructions);
                if (err != 0) {
                    printf("constant %d - test_instructions failed\n", i);
                    return -1;
                }
                break;

            default:
                printf("test_constants - constant type %d not implemented\n", exp.typ);
                TEST_FAIL();
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
    RUN_TEST(test_table_literals);
    RUN_TEST(test_index_expressions);
    RUN_TEST(test_functions);
    RUN_TEST(test_functions_without_return_value);
    RUN_TEST(test_compiler_scopes);
    RUN_TEST(test_function_calls);
    RUN_TEST(test_let_statements_scopes);
    RUN_TEST(test_builtins);
    RUN_TEST(test_closures);
    RUN_TEST(test_recursive_functions);
    RUN_TEST(test_assignments);
    RUN_TEST(test_operator_assignments);
    RUN_TEST(test_return_statements);
    RUN_TEST(test_loops);
    RUN_TEST(test_source_mappings);
    RUN_TEST(test_modules);
    return UNITY_END();
}
