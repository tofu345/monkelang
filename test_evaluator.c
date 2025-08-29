#include "unity/unity.h"
#include "object.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static void
check_parser_errors(Parser* p) {
    if (p->errors_len == 0) {
        return;
    }

    printf("parser had %d errors\n", (int)p->errors_len);
    for (size_t i = 0; i < p->errors_len; i++) {
        printf("parser error: %s\n", p->errors[i]);
    }
    TEST_FAIL();
}

static Object
test_eval(char* input, size_t input_len) {
    Lexer l = lexer_new(input, input_len);
    Parser p;
    parser_init(&p, &l);
    Program prog = parse_program(&p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");
    check_parser_errors(&p);

    Object evaluated = eval_program(&prog);

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
        Object evaluated = test_eval(test.input, strlen(test.input));
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
        Object evaluated = test_eval(test.input, strlen(test.input));
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
        Object evaluated = test_eval(test.input, strlen(test.input));
        test_boolean_object(evaluated, test.expected);
        object_destroy(&evaluated);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_eval_integer_expression);
    RUN_TEST(test_eval_boolean_expression);
    RUN_TEST(test_bang_operator);
    return UNITY_END();
}
