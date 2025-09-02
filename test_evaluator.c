#include "ast.h"
#include "environment.h"
#include "unity/unity.h"
#include "object.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static void
check_parser_errors(Parser* p) {
    if (p->errors.len == 0) {
        return;
    }

    printf("parser had %d errors\n", (int)p->errors.len);
    for (size_t i = 0; i < p->errors.len; i++) {
        printf("parser error: %s\n", p->errors.data[i]);
    }
    TEST_FAIL();
}

static Object
test_eval(char* input) {
    Lexer l = lexer_new(input);
    Parser p;
    parser_init(&p, &l);
    Program prog = parse_program(&p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    Env* env = env_new();
    TEST_ASSERT_NOT_NULL_MESSAGE(env, "env NULL");
    Object evaluated = eval_program(&prog, env);
    // env_destroy(env);
    program_destroy(&prog);
    parser_destroy(&p);
    return evaluated;
}

static void
test_integer_object(Object o, long expected) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
		show_object_type(o_Integer), show_object_type(o.typ),
        "incorrect type");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            expected, o.data.integer, "integer has wrong value");
}

void test_eval_integer_expression(void) {
    struct Test {
        char* input;
        long expected;
    } tests[] = {
        {"5", 5},
        {"10", 10},
        {"-5", -5},
        {"-10", -10},
        {"5 + 5 + 5 + 5 - 10", 10},
        {"2 * 2 * 2 * 2 * 2", 32},
        {"-50 + 100 + -50", 0},
        {"5 * 2 + 10", 20},
        {"5 + 2 * 10", 25},
        {"20 + 2 * -10", 0},
        {"50 / 2 * 2 + 10", 60},
        {"2 * (5 + 10)", 30},
        {"3 * 3 * 3 + 10", 37},
        {"3 * (3 * 3) + 10", 37},
        {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Object evaluated = test_eval(test.input);
        test_integer_object(evaluated, test.expected);
        object_destroy(&evaluated);
    }
}

static void
test_boolean_object(Object o, bool expected) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
		show_object_type(o_Boolean), show_object_type(o.typ),
        "incorrect type");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            expected, o.data.boolean, "boolean has wrong value");
}

void test_eval_boolean_expression(void) {
    struct Test {
        char* input;
        bool expected;
    } tests[] = {
        {"true", true},
        {"false", false},
        {"1 < 2", true},
        {"1 > 2", false},
        {"1 < 1", false},
        {"1 > 1", false},
        {"1 == 1", true},
        {"1 != 1", false},
        {"1 == 2", false},
        {"1 != 2", true},
        {"true == true", true},
        {"false == false", true},
        {"true == false", false},
        {"true != false", true},
        {"false != true", true},
        {"(1 < 2) == true", true},
        {"(1 < 2) == false", false},
        {"(1 > 2) == true", false},
        {"(1 > 2) == false", true},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Object evaluated = test_eval(test.input);
        test_boolean_object(evaluated, test.expected);
        object_destroy(&evaluated);
    }
}

void test_bang_operator(void) {
    struct Test {
        char* input;
        bool expected;
    } tests[] = {
        {"!true", false},
        {"!false", true},
        {"!5", false},
        {"!!true", true},
        {"!!false", false},
        {"!!5", true},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Object evaluated = test_eval(test.input);
        test_boolean_object(evaluated, test.expected);
        object_destroy(&evaluated);
    }
}

void test_null_object(Object o) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "Null", show_object_type(o.typ),
            "object is not null");
}

void test_if_else_expression(void) {
    struct Test {
        char* input;
        long expected;
    } tests[] = {
        {"if (true) { 10 }", 10},
        {"if (false) { 10 }", 0}, // NULL
        {"if (1) { 10 }", 10},
        {"if (1 < 2) { 10 }", 10},
        {"if (1 > 2) { 10 }", 0}, // NULL
        {"if (1 > 2) { 10 } else { 20 }", 20},
        {"if (1 < 2) { 10 } else { 20 }", 10},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Object evaluated = test_eval(test.input);
        if (test.expected == 0) {
            test_null_object(evaluated);
        } else {
            test_integer_object(evaluated, test.expected);
        }
        object_destroy(&evaluated);
    }
}

void test_return_statements(void) {
    struct Test {
        char* input;
        long expected;
    } tests[] = {
        {"return 10;", 10},
        {"return 10; 9;", 10},
        {"return 2 * 5; 9;", 10},
        {"9; return 2 * 5; 9;", 10},
        {
            "if (10 > 1) {\
                if (10 > 1) {\
                    return 10;\
                }\
                return 1;\
            }",
            10,
        },
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Object evaluated = test_eval(test.input);
        test_integer_object(evaluated, test.expected);
        object_destroy(&evaluated);
    }
}

void test_error_handling(void) {
    struct Test {
        char* input;
        char* error_msg;
    } tests[] = {
        // {
        //     "5 + true;",
        //     "type mismatch: Integer + Boolean",
        // },
        // {
        //     "5 + true; 5;",
        //     "type mismatch: Integer + Boolean",
        // },
        // {
        //     "-true",
        //     "unknown operator: -Boolean",
        // },
        // {
        //     "true + false;",
        //     "unknown operator: Boolean + Boolean",
        // },
        // {
        //     "5; true + false; 5",
        //     "unknown operator: Boolean + Boolean",
        // },
        // {
        //     "if (10 > 1) { true + false; }",
        //     "unknown operator: Boolean + Boolean",
        // },
        // {
        //     "if (10 > 1) {\
        //         if (10 > 1) {\
        //             return true + false;\
        //         }\
        //     \
        //         return 1;\
        //     }",
        //     "unknown operator: Boolean + Boolean",
        // },
        {
            "foobar",
            "identifier not found: foobar",
        },
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Object error = test_eval(test.input);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                show_object_type(o_Error),
                show_object_type(error.typ), "wrong object type");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.error_msg, error.data.error_msg,
                "error has wrong message");
        object_destroy(&error);
    }
}

void test_let_statements(void) {
    struct Test {
        char* input;
        long expected;
    } tests[] = {
        {"let a = 5; a;", 5},
        {"let a = 5 * 5; a;", 25},
        {"let a = 5; let b = a; b;", 5},
        {"let a = 5; let b = a; let c = a + b + 5; c;", 15},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Object evaluated = test_eval(test.input);
        test_integer_object(evaluated, test.expected);
        object_destroy(&evaluated);
    }
}

void test_function_object(void) {
    char* input = "fn(x) { x + 2; };";
    Lexer l = lexer_new(input);
    Parser p;
    parser_init(&p, &l);
    Program prog = parse_program(&p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    Env* env = env_new();
    Object evaluated = eval_program(&prog, env);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            show_object_type(o_Function), show_object_type(evaluated.typ),
            "wrong object type");

    Function* fn = evaluated.data.func;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, fn->len, "wrong number of parameters");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "x", fn->params[0]->value, "wrong paramater");

    char* expected_body = "(x + 2)";

    char* buf = NULL;
    size_t len;
    FILE* fp = open_memstream(&buf, &len);
    int err = node_fprint((Node){ n_BlockStatement, fn->body }, fp);
    if (err == -1) TEST_FAIL_MESSAGE("node_fprint fail");
    fclose(fp);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            expected_body, buf, "wrong function body");

    free(buf);
    object_destroy(&evaluated);
    program_destroy(&prog);
    parser_destroy(&p);
}

void test_function_application(void) {
    struct Test {
        char* input;
        long expected;
    } tests[] = {
        {"let identity = fn(x) { x; }; identity(5);", 5},
        {"let identity = fn(x) { return x; }; identity(5);", 5},
        {"let double = fn(x) { x * 2; }; double(5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
        {"fn(x) { x; }(5)", 5},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Object evaluated = test_eval(test.input);
        test_integer_object(evaluated, test.expected);
        object_destroy(&evaluated);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_eval_integer_expression);
    RUN_TEST(test_eval_boolean_expression);
    RUN_TEST(test_bang_operator);
    RUN_TEST(test_if_else_expression);
    RUN_TEST(test_return_statements);
    RUN_TEST(test_error_handling);
    RUN_TEST(test_function_object);
    RUN_TEST(test_function_application);
    return UNITY_END();
}
