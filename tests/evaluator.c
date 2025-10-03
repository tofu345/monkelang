#include "unity/unity.h"

#include "tests.h"
#include "../src/ast.h"
#include "../src/environment.h"
#include "../src/object.h"
#include "../src/evaluator.h"
#include "../src/lexer.h"
#include "../src/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static void
check_parser_errors(Parser* p) {
    if (p->errors.length == 0) {
        return;
    }

    printf("parser had %d errors\n", p->errors.length);
    for (int i = 0; i < p->errors.length; i++) {
        printf("%s\n", p->errors.data[i]);
    }
    TEST_FAIL();
}

static Object*
test_eval(char* input, Env* env) {
    Lexer l = lexer_new(input);
    Parser p;
    parser_init(&p, &l);
    Program prog = parse_program(&p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    Object* result = eval_program(&prog, env);
    program_destroy(&prog);
    parser_destroy(&p);
    return result;
}

static void
test_integer_object(Object* o, long expected) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
		show_object_type(o_Integer), show_object_type(o->typ),
        "incorrect type");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            expected, o->data.integer, "integer has wrong value");
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
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        test_integer_object(evaluated, test.expected);
        env_destroy(env);
    }
}

static void
test_boolean_object(Object* o, bool expected) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
		show_object_type(o_Boolean), show_object_type(o->typ),
        "incorrect type");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            expected, o->data.boolean, "boolean has wrong value");
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
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        test_boolean_object(evaluated, test.expected);
        env_destroy(env);
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
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        test_boolean_object(evaluated, test.expected);
        env_destroy(env);
    }
}

void test_null_object(Object* o) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "Null", show_object_type(o->typ),
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
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        if (test.expected == 0) {
            test_null_object(evaluated);
        } else {
            test_integer_object(evaluated, test.expected);
        }
        env_destroy(env);
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
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        test_integer_object(evaluated, test.expected);
        env_destroy(env);
    }
}

void test_error_handling(void) {
    struct Test {
        char* input;
        char* error_msg;
    } tests[] = {
        {
            "5 + true;",
            "type mismatch: Integer + Boolean",
        },
        {
            "5 + true; 5;",
            "type mismatch: Integer + Boolean",
        },
        {
            "-true",
            "unknown operator: -Boolean",
        },
        {
            "true + false;",
            "unknown operator: Boolean + Boolean",
        },
        {
            "5; true + false; 5",
            "unknown operator: Boolean + Boolean",
        },
        {
            "if (10 > 1) { true + false; }",
            "unknown operator: Boolean + Boolean",
        },
        {
            "if (10 > 1) {\
                if (10 > 1) {\
                    return true + false;\
                }\
            \
                return 1;\
            }",
            "unknown operator: Boolean + Boolean",
        },
        {
            "foobar",
            "identifier not found: foobar",
        },
        {
            "\"Hello\" - \"World\"",
            "unknown operator: String - String",
        },
        {
            "{\"name\": \"Monkey\"}[fn(x) { x }];",
            "unusable as hash key: Function",
        },
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Env* env = env_new();
        Object* error = test_eval(test.input, env);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                show_object_type(o_Error),
                show_object_type(error->typ), "wrong object type");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.error_msg, error->data.error_msg,
                "error has wrong message");
        env_destroy(env);
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
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        test_integer_object(evaluated, test.expected);
        env_destroy(env);
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
    Object* evaluated = eval_program(&prog, env);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            show_object_type(o_Function), show_object_type(evaluated->typ),
            "wrong object type");

    FunctionLiteral* fn = evaluated->data.func;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, fn->params.length, "wrong number of parameters");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "x", fn->params.data[0]->tok.literal, "wrong paramater");

    char* expected_body = "(x + 2);";

    char* buf = NULL;
    size_t len;
    FILE* fp = open_memstream(&buf, &len);
    int err = node_fprint((Node){ n_BlockStatement, fn->body }, fp);
    if (err == -1) TEST_FAIL_MESSAGE("node_fprint fail");
    fclose(fp);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            expected_body, buf, "wrong function body");

    free(buf);
    env_destroy(env);
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
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        test_integer_object(evaluated, test.expected);
        env_destroy(env);
    }
}

void test_closures(void) {
    char* input = " \
let newAdder = fn(x) { \
    fn(y) { x + y }; \
}; \
let addTwo = newAdder(2); \
addTwo(2);\
addTwo(2);\
    ";
    Env* env = env_new();
    Object* evaluated = test_eval(input, env);
    test_integer_object(evaluated, 4);
    env_destroy(env);
}

void test_string_literal(void) {
    char* input = "\"Hello World!\"";
    Env* env = env_new();
    Object* evaluated = test_eval(input, env);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            show_object_type(o_String), show_object_type(evaluated->typ),
            "type not String");
    char* data = evaluated->data.string->data;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "Hello World!", data,
            "String has wrong value");
    env_destroy(env);
}

void test_string_concatenation(void) {
    char* input = "\"Hello\" + \" \" + \"World!\"";
    Env* env = env_new();
    Object* evaluated = test_eval(input, env);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            show_object_type(o_String),
            show_object_type(evaluated->typ), "type not String");
    char* data = evaluated->data.string->data;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "Hello World!", data,
            "String has wrong value");
    env_destroy(env);
}

void test_builtin_function(void) {
    struct Test {
        char* input;
        Test expected;
    } tests[] = {
        {"len(\"\")", TEST(int, 0)},
        {"len(\"four\")", TEST(int, 4)},
        {"len(\"hello world\")", TEST(int, 11)},
        {
            "len(1)",
            TEST(str, "builtin len(): argument of 'Integer' not supported")
        },
        {
            "len(\"one\", \"two\")",
            TEST(str, "builtin len() takes 1 argument got 2"),
        },
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        switch (test.expected.typ) {
            case test_int:
                test_integer_object(evaluated, test.expected.val._int);
                break;
            case test_str:
                TEST_ASSERT_EQUAL_STRING_MESSAGE(
                        show_object_type(o_Error),
                        show_object_type(evaluated->typ),
                        "type not Error");
                TEST_ASSERT_EQUAL_STRING_MESSAGE(
                        test.expected.val._str, evaluated->data.error_msg,
                        "integer has wrong value");
                break;
            default: TEST_FAIL_MESSAGE("unhandled type");
        }
        env_destroy(env);
    }
}

void test_array_literals(void) {
    char* input = "[1, 2 * 2, 3 + 3]";
    Env* env = env_new();
    Object* evaluated = test_eval(input, env);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            show_object_type(o_Array), show_object_type(evaluated->typ),
            "type not Array");
    ObjectBuffer* arr = evaluated->data.array;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, arr->length, "wrong number of elements");
    test_integer_object(arr->data[0], 1);
    test_integer_object(arr->data[1], 4);
    test_integer_object(arr->data[2], 6);
    env_destroy(env);
}

void test_array_index_expressions(void) {
    struct Test {
	char* input;
	long expected;
    } tests[] = {
	{
	    "[1, 2, 3][0]",
	    1,
	},
	{
	    "[1, 2, 3][1]",
	    2,
	},
	{
	    "[1, 2, 3][2]",
	    3,
	},
	{
	    "let i = 0; [1][i];",
	    1,
	},
	{
	    "[1, 2, 3][1 + 1];",
	    3,
	},
	{
	    "let myArray = [1, 2, 3]; myArray[2];",
	    3,
	},
	{
	    "let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];",
	    6,
	},
	{
	    "let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i]",
	    2,
	},
	{
	    "[1, 2, 3][3]",
	    0, // NULL
	},
	{
	    "[1, 2, 3][-1]",
	    0, // NULL
	},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
	struct Test test = tests[i];
	Env* env = env_new();
	Object* evaluated = test_eval(test.input, env);
	if (test.expected == 0) {
	    test_null_object(evaluated);
	} else {
	    test_integer_object(evaluated, test.expected);
	}
	env_destroy(env);
    }
}

void test_hash_literals(void) {
    char* input = "let two = \"two\";\n\
    {\n\
        \"one\": 10 - 9,\n\
        two: 1 + 1,\n\
        \"thr\" + \"ee\": 6 / 2,\n\
        \"4\": 4,\n\
        \"true\": 5,\n\
        \"false\": 6\n\
    }";
    Env* env = env_new();
    Object* evaluated = test_eval(input, env);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            show_object_type(o_Hash), show_object_type(evaluated->typ),
            "type not Hash");

    ht* pairs = evaluated->data.hash;

    struct Test {
        char* key;
        long value;
    } expected[] = {
        {"one", 1},
        {"two", 2},
        {"three", 3},
        {"4", 4},
        {"true", 5},
        {"false", 6},
    };
    int expected_len = sizeof(expected) / sizeof(expected[0]);

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            expected_len, pairs->length, "wrong number of elements");

    for (int i = 0; i < expected_len; i++) {
        Object* val = ht_get(pairs, expected[i].key);
        if (val == NULL)
            TEST_FAIL_MESSAGE("no pair for given key in Pairs");

        test_integer_object(val, expected[i].value);
    }

    env_destroy(env);
}

void test_hash_index_expressions(void) {
    struct Test {
        char* input;
        long expected;
    } tests[] = {
        {
            "{\"foo\": 5}[\"foo\"]",
            5,
        },
        {
            "{\"foo\": 5}[\"bar\"]",
            0, // NULL
        },
        {
            "let key = \"foo\"; {\"foo\": 5}[key]",
            5,
        },
        {
            "{}[\"foo\"]",
            0, // NULL
        },
        {
            "{\"5\": 5}[\"5\"]",
            5,
        },
        {
            "{\"true\": 5}[\"true\"]",
            5,
        },
        {
            "{\"false\": 5}[\"false\"]",
            5,
        },
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        Env* env = env_new();
        Object* evaluated = test_eval(test.input, env);
        if (test.expected == 0) {
            test_null_object(evaluated);
        } else {
            test_integer_object(evaluated, test.expected);
        }
        env_destroy(env);
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
    RUN_TEST(test_closures);
    RUN_TEST(test_string_literal);
    RUN_TEST(test_string_concatenation);
    RUN_TEST(test_builtin_function);
    RUN_TEST(test_array_literals);
    RUN_TEST(test_array_index_expressions);
    RUN_TEST(test_hash_literals);
    RUN_TEST(test_hash_index_expressions);
    return UNITY_END();
}
