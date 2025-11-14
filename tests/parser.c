#include "unity/unity.h"
#include "helpers.h"

#include "../src/ast.h"
#include "../src/parser.h"
#include "../src/token.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Parser p;
Program prog;

void setUp(void) {}

void tearDown(void) {
    program_free(&prog);
    parser_free(&p);
}

// free and reset [prog]
static void
__program_free(void) {
    program_free(&prog);
    prog = (Program){0};
}

static void check_parser_errors(Parser* p) {
    if (p->errors.length == 0) {
        return;
    }

    print_parser_errors(p);
    TEST_FAIL();
}

static void test_let_statement(Node stmt, const char* exp_name) {
    Token *tok = stmt.obj;
    if (strncmp("let", tok->start, tok->length)) {
        printf("wrong LetStatement.Token.literal %.*s\n", tok->length,
                tok->start);
        TEST_FAIL();
    }

    TEST_ASSERT_MESSAGE(n_LetStatement == stmt.typ, "type not LetStatement");

    LetStatement* let_stmt = (LetStatement*)stmt.obj;
    if (strncmp(exp_name, let_stmt->name->tok.start, let_stmt->name->tok.length)) {
        puts("wrong LetStatement.Identifier.Value");
        TEST_FAIL();
    }
}

void test_return_statements(void) {
    char* input = "\
return;\
return 5;\
return 10;\
return 993322;\
";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            4, prog.stmts.length, "wrong prog.statements length");

    for (int i = 0; i < prog.stmts.length; i++) {
        Node stmt = prog.stmts.data[i];

        if (stmt.typ != n_ReturnStatement) {
            printf("type not ReturnStatement got='%d'\n", stmt.typ);
            TEST_FAIL();
        }

        if (strncmp("return", ((Token*)stmt.obj)->start, ((Token*)stmt.obj)->length)) {
            puts("wrong ReturnStatement.Token.literal");
            TEST_FAIL();
        }
    }
}

void test_identifier_expression(void) {
    char* input = "foobar;";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    Node n = prog.stmts.data[0];
    TEST_ASSERT_MESSAGE(
            n_ExpressionStatement == n.typ, "type not ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_MESSAGE(
            n_Identifier == es->expression.typ, "type not Identifier");

    Identifier* ident = es->expression.obj;
    if (strncmp("foobar", ident->tok.start, ident->tok.length)) {
        puts("wrong Identifier.Token.value");
        TEST_FAIL();
    }
}

static void
_test_integer_literal(Node n, long value, char* expected) {
    TEST_ASSERT_MESSAGE(n_IntegerLiteral == n.typ, "type not IntegerLiteral");

    IntegerLiteral* il = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            value, il->value, "wrong IntegerLiteral.value");

    if (strncmp(expected, il->tok.start, il->tok.length)) {
        puts("wrong IntegerLiteral.tok.literal");
        TEST_FAIL();
    }
}

static void
test_integer_literal(Node n, long value) {
    char* expected = NULL;
    if (asprintf(&expected, "%ld", value) == -1)
        TEST_FAIL_MESSAGE("no memory");

    _test_integer_literal(n, value, expected);
    free(expected);
}

static void
test_float_literal(Node n, double value) {
    TEST_ASSERT_MESSAGE(n_FloatLiteral == n.typ, "type not FloatLiteral");

    FloatLiteral* il = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            value, il->value, "wrong FloatLiteral.value");

    char* expected = NULL;
    if (asprintf(&expected, "%.3f", value) == -1)
        TEST_FAIL_MESSAGE("no memory");

    if (strncmp(expected, il->tok.start, il->tok.length)) {
        puts("wrong FloatLiteral.tok.literal");
        TEST_FAIL();
    }
    free(expected);
}

void test_integer_literal_expression(void) {
    char* input = "5;";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    Node n = prog.stmts.data[0];
    TEST_ASSERT_MESSAGE(
            n_ExpressionStatement == n.typ, "type not ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IntegerLiteral, es->expression.typ, "wrong IntegerLiteral");

    IntegerLiteral* int_lit = es->expression.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            5, int_lit->value, "wrong IntegerLiteral.value");

    if (strncmp("5", int_lit->tok.start, int_lit->tok.length)) {
        puts("wrong IntegerLiteral.Token.literal");
        TEST_FAIL();
    }
}

void test_float_literal_expression(void) {
    char* input = "5.01;";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    Node n = prog.stmts.data[0];
    TEST_ASSERT_MESSAGE(
            n_ExpressionStatement == n.typ, "type not ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_FloatLiteral, es->expression.typ, "wrong FloatLiteral");

    FloatLiteral* fl_lit = es->expression.obj;
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(
            5.01, fl_lit->value, "wrong FloatLiteral.value");

    if (strncmp("5.01", fl_lit->tok.start, fl_lit->tok.length)) {
        puts("wrong FloatLiteral.Token.literal");
        TEST_FAIL();
    }
}

static void
test_identifier(Node n, const char* value) {
    TEST_ASSERT_MESSAGE(n_Identifier == n.typ, "type not type Identifier");
    Identifier* id = n.obj;
    if (strncmp(value, id->tok.start, id->tok.length)) {
        puts("wrong Identifier.Value");
        TEST_FAIL();
    }
}

static void
test_boolean_literal(Node n, bool exp) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_BooleanLiteral, n.typ, "wrong type BooleanLiteral");
    BooleanLiteral* b = n.obj;
    TEST_ASSERT_MESSAGE(b->value == exp, "wrong BooleanLiteral.value");
    if (strncmp(exp ? "true" : "false", b->tok.start, b->tok.length)) {
        puts("wrong BooleanLiteral.Token.literal");
        TEST_FAIL();
    }
}

static void
test_literal_expression(Node n, Test *test) {
    switch (test->typ) {
        case test_int:
            test_integer_literal(n, test->val._int);
            break;
        case test_float:
            test_float_literal(n, test->val._float);
            break;
        case test_str:
            test_identifier(n, test->val._str);
            break;
        case test_bool:
            test_boolean_literal(n, test->val._bool);
            break;
        default:
            fprintf(stderr, "type of exp not handled. got %d", test->typ);
            exit(1);
    }
}

void test_let_statements(void) {
    struct Test {
        const char* input;
        const char* expectedIdent;
        Test *expectedVal;
    } tests[] = {
        {"let x = 5;", "x", TEST(int, 5)},
        {"let x = 5.000;", "x", TEST(float, 5.0)},
        {"let y = true;", "y", TEST(bool, true)},
        {"let foobar = y;", "foobar", TEST(str, "y")},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        prog = parse(&p, test.input);

        check_parser_errors(&p);

        Node stmt = prog.stmts.data[0];
        test_let_statement(stmt, test.expectedIdent);

        LetStatement* ls = stmt.obj;
        test_literal_expression(ls->value, test.expectedVal);

        __program_free();
    }

    prog = (Program){0};
}

void test_parsing_prefix_expressions(void) {
    struct Test {
        char* input;
        char* operator;
        Test *value;
    } tests[] = {
        {"!5;", "!", TEST(int, 5)},
        {"-15;", "-", TEST(int, 15)},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        prog = parse(&p, test.input);

        check_parser_errors(&p);
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                1, prog.stmts.length, "wrong prog.statements length");

        Node n = prog.stmts.data[0];
        TEST_ASSERT_MESSAGE(
                n_ExpressionStatement == n.typ,
                "type not ExpressionStatement");

        ExpressionStatement* es = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_PrefixExpression, es->expression.typ,
                "wrong PrefixExpression");

        PrefixExpression* pe = es->expression.obj;
        if (strncmp(test.operator, pe->tok.start, pe->tok.length)) {
            puts("wrong PrefixExpression.op");
            TEST_FAIL();
        }

        test_literal_expression(pe->right, test.value);

        __program_free();
    }

    prog = (Program){0};
}

void test_parsing_infix_expressions(void) {
    struct Test {
        char* input;
        Test *left_value;
        char* operator;
        Test *right_value;
    } tests[] = {
        {"5 + 5;", TEST(int, 5), "+", TEST(int, 5)},
        {"5 - 5;", TEST(int, 5), "-", TEST(int, 5)},
        {"5 * 5;", TEST(int, 5), "*", TEST(int, 5)},
        {"5 / 5;", TEST(int, 5), "/", TEST(int, 5)},
        {"5 > 5;", TEST(int, 5), ">", TEST(int, 5)},
        {"5 < 5;", TEST(int, 5), "<", TEST(int, 5)},
        {"5 == 5;", TEST(int, 5), "==", TEST(int, 5)},
        {"5 != 5;", TEST(int, 5), "!=", TEST(int, 5)},
        {"true == true", TEST(bool, true), "==", TEST(bool, true)},
        {"true != false", TEST(bool, true), "!=", TEST(bool, false)},
        {"false == false", TEST(bool, false), "==", TEST(bool, false)},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        prog = parse(&p, test.input);

        check_parser_errors(&p);
        TEST_ASSERT_EQUAL_INT_MESSAGE
            (1, prog.stmts.length, "wrong prog.statements length");

        Node n = prog.stmts.data[0];
        TEST_ASSERT_MESSAGE
            (n_ExpressionStatement == n.typ, "type not ExpressionStatement");

        ExpressionStatement* es = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_InfixExpression, es->expression.typ,
                "wrong InfixExpression");

        InfixExpression* ie = es->expression.obj;

        test_literal_expression(ie->left, test.left_value);

        if (strncmp(test.operator, ie->tok.start, ie->tok.length)) {
            puts("wrong InfixExpression.op");
            TEST_FAIL();
        }

        test_literal_expression(ie->right, test.right_value);

        __program_free();
    }

    prog = (Program){0};
}

void test_operator_precedence_parsing(void) {
    struct Test {
        char* input;
        char* expected;
    } tests[] = {
        {
            "-1 * 2 + 3",
            "(((-1) * 2) + 3);",
        },
        {
            "-a * b",
            "((-a) * b);",
        },
        {
            "!-a",
            "(!(-a));",
        },
        {
            "a + b + c",
            "((a + b) + c);",
        },
        {
            "a + b - c",
            "((a + b) - c);",
        },
        {
            "a * b * c",
            "((a * b) * c);",
        },
        {
            "a * b / c",
            "((a * b) / c);",
        },
        {
            "a + b / c",
            "(a + (b / c));",
        },
        {
            "a + b * c + d / e - f",
            "(((a + (b * c)) + (d / e)) - f);",
        },
        {
            "3 + 4; -5 * 5",
            "(3 + 4);((-5) * 5);",
        },
        {
            "5 > 4 == 3 < 4",
            "((5 > 4) == (3 < 4));",
        },
        {
            "5 < 4 != 3 > 4",
            "((5 < 4) != (3 > 4));",
        },
        {
            "3 + 4 * 5 == 3 * 1 + 4 * 5",
            "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)));",
        },
        {
            "true",
            "true;",
        },
        {
            "false",
            "false;",
        },
        {
            "3 > 5 == false",
            "((3 > 5) == false);",
        },
        {
            "3 < 5 == true",
            "((3 < 5) == true);",
        },
        {
            "1 + (2 + 3) + 4",
            "((1 + (2 + 3)) + 4);",
        },
        {
            "(5 + 5) * 2",
            "((5 + 5) * 2);",
        },
        {
            "2 / (5 + 5);",
            "(2 / (5 + 5));",
        },
        {
            "-(5 + 5);",
            "(-(5 + 5));",
        },
        {
            "-(1.1 + 5);",
            "(-(1.100 + 5));",
        },
        {
            "-(2. * 5);",
            "(-(2.000 * 5));",
        },
        {
            "!(true == true);",
            "(!(true == true));",
        },
        {
            "a + add(b * c) + d",
            "((a + add((b * c))) + d);",
        },
        {
            "add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 * 8));",
            "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)));",
        },
        {
            "add(a + b + c * d / f + g);",
            "add((((a + b) + ((c * d) / f)) + g));",
        },
        {
            "a * [1, 2, 3, 4][b * c] * d",
            "((a * [1, 2, 3, 4][(b * c)]) * d);",
        },
        {
            "add(a * b[2], b[1], 2 * [1, 2][1])",
            "add((a * b[2]), b[1], (2 * [1, 2][1]));",
        },
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        prog = parse(&p, test.input);

        check_parser_errors(&p);

        Node n = prog.stmts.data[0];
        TEST_ASSERT_MESSAGE
            (n_ExpressionStatement == n.typ, "type not ExpressionStatement");

        int len = strlen(test.expected) + 2;
        char* buf = calloc(len, sizeof(char));
        FILE* fp = fmemopen(buf, len, "w");
        if (fp == NULL) {
            fprintf(stderr, "no memory");
            exit(1);
        }

        TEST_ASSERT_MESSAGE(
                program_fprint(&prog, fp) != -1, "program_fprint fail");

        fclose(fp);

        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.expected, buf, "program_fprint wrong");

        free(buf);

        __program_free();
    }

    prog = (Program){0};
}

static void
test_infix_expression(Node n, Test *left, char* operator, Test *right) {
    TEST_ASSERT_MESSAGE
        (n_InfixExpression == n.typ, "type not type InfixExpression");

    InfixExpression* ie = n.obj;
    test_literal_expression(ie->left, left);
    if (strncmp(operator, ie->tok.start, ie->tok.length)) {
        puts("wrong InfixExpression.op");
        TEST_FAIL();
    }
    test_literal_expression(ie->right, right);
}

void test_boolean_expression(void) {
    struct Test {
        char* input;
        bool expected;
    } tests[] = {
        {"true;", true},
        {"false;", false},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        prog = parse(&p, test.input);

        check_parser_errors(&p);

        TEST_ASSERT_EQUAL_INT_MESSAGE
            (1, prog.stmts.length, "wrong prog.statements length");

        Node n = prog.stmts.data[0];
        TEST_ASSERT_MESSAGE
            (n_ExpressionStatement == n.typ, "type not ExpressionStatement");

        ExpressionStatement* es = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_BooleanLiteral, es->expression.typ,
                "wrong BooleanLiteral type");

        BooleanLiteral* b = es->expression.obj;
        TEST_ASSERT_MESSAGE(
                b->value == test.expected, "wrong BooleanLiteral.value");


        __program_free();
    }

    prog = (Program){0};
}

void test_if_expression(void) {
    char* input = "if (x < y) { x }";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE
        (1, prog.stmts.length, "wrong prog.statements length");

    Node n = prog.stmts.data[0];
    TEST_ASSERT_MESSAGE
        (n_ExpressionStatement == n.typ, "type not ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IfExpression, es->expression.typ, "wrong IfExpression");

    IfExpression* ie = es->expression.obj;
    test_infix_expression(ie->condition, TEST(str, "x"), "<", TEST(str, "y"));

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ie->consequence->stmts.length,
            "wrong IfExpression.consequence length");
    TEST_ASSERT_MESSAGE(
            n_ExpressionStatement == ie->consequence->stmts.data[0].typ,
            "IfExpression.consequence.statements[0].typ ExpressionStatement");

    ExpressionStatement* consequence = ie->consequence->stmts.data[0].obj;
    test_identifier(consequence->expression, "x");

    TEST_ASSERT_NULL_MESSAGE(ie->alternative,
            "IfExpression.alternative should be NULL");
}

void test_if_else_expression(void) {
    char* input = "if (x < y) { x } else { y }";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE
        (1, prog.stmts.length, "wrong prog.statements length");

    Node n = prog.stmts.data[0];
    TEST_ASSERT_MESSAGE
        (n_ExpressionStatement == n.typ, "type not ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IfExpression, es->expression.typ, "wrong IfExpression");

    IfExpression* ie = es->expression.obj;
    test_infix_expression(ie->condition, TEST(str, "x"), "<", TEST(str, "y"));

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ie->consequence->stmts.length,
            "wrong IfExpression.consequence length");
    TEST_ASSERT_MESSAGE(
            n_ExpressionStatement == ie->consequence->stmts.data[0].typ,
            "IfExpression.consequence.statements[0].typ is not ExpressionStatement");

    ExpressionStatement* consequence = ie->consequence->stmts.data[0].obj;
    test_identifier(consequence->expression, "x");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ie->alternative->stmts.length,
            "IfExpression.alternative length");
    TEST_ASSERT_MESSAGE(
            n_ExpressionStatement == ie->alternative->stmts.data[0].typ,
            "IfExpression.alternative.statements[0].typ is not ExpressionStatement");

    ExpressionStatement* alternative = ie->alternative->stmts.data[0].obj;
    test_identifier(alternative->expression, "y");
}

void test_function_literal_parsing(void) {
    char* input = "fn(x, y) { x + y; }";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE
        (1, prog.stmts.length, "wrong prog.statements length");

    Node n = prog.stmts.data[0];
    TEST_ASSERT_MESSAGE
        (n_ExpressionStatement == n.typ, "type not ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_FunctionLiteral, es->expression.typ, "wrong n_FunctionLiteral");

    FunctionLiteral* fl = es->expression.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            2, fl->params.length, "wrong FunctionLiteral.parems.length");

    test_literal_expression(
	    (Node){ n_Identifier, fl->params.data[0] }, TEST(str, "x"));
    test_literal_expression(
	    (Node){ n_Identifier, fl->params.data[1] }, TEST(str, "y"));

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, fl->body->stmts.length,
            "wrong FunctionLiteral.body.len");

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, fl->body->stmts.data[0].typ,
            "wrong ExpressionStatement");

    ExpressionStatement* body_stmt = fl->body->stmts.data[0].obj;

    test_infix_expression(body_stmt->expression,
            TEST(str, "x"), "+", TEST(str, "y"));
}

void test_function_literal_with_name(void) {
    char* input = "let myFunction = fn() { };";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.stmts.length,
            "wrong prog.statements length");

    Node n = prog.stmts.data[0];
    TEST_ASSERT_MESSAGE(n_LetStatement == n.typ,
            "type not LetStatement");

    LetStatement* ls = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_FunctionLiteral, ls->value.typ,
            "LetStatement Value is not FunctionLiteral");

    FunctionLiteral* fl = ls->value.obj;

    Token tok = fl->name->tok;
    if(strncmp("myFunction", tok.start, tok.length) != 0) {
        printf("function literal name wrong. want 'myFunction', got='%.*s'\n",
                tok.length, tok.start);
        TEST_FAIL();
    }
}

void test_function_parameter_parsing(void) {
    struct Test {
        char* input;
        char* expectedParams[3];
        int len;
    } tests[] = {
        {"fn() {};", {0}, 0},
        {"fn(x) {};", {"x"}, 1},
        {"fn(x, y, z) {};", {"x", "y", "z"}, 3},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        prog = parse(&p, test.input);

        check_parser_errors(&p);

        TEST_ASSERT_EQUAL_INT_MESSAGE(
                1, prog.stmts.length, "wrong prog.statements length");

        // segfaults waiting to happen
        Node n = prog.stmts.data[0];
        ExpressionStatement* es = n.obj;
        FunctionLiteral* fl = es->expression.obj;

        TEST_ASSERT_EQUAL_INT_MESSAGE(
                test.len, fl->params.length,
                "wrong FunctionLiteral.paremeters_len");

        for (int i = 0; i < test.len; i++) {
            test_literal_expression(
		    (Node){ n_Identifier, fl->params.data[i] },
                    TEST(str, test.expectedParams[i]));
        }

        __program_free();
    }

    prog = (Program){0};
}

void test_call_expression_parsing(void) {
    char* input = "add(1, 2 * 3, 4 + 5);";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    Node n = prog.stmts.data[0];
    TEST_ASSERT_MESSAGE(
            n_ExpressionStatement == n.typ, "type not ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_CallExpression, es->expression.typ, "wrong n_CallExpression");

    CallExpression* ce = es->expression.obj;

    test_identifier(ce->function, "add");

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, ce->args.length, "wrong CallExpression.args_len");

    test_literal_expression(
            ce->args.data[0], TEST(int, 1));

    test_infix_expression(ce->args.data[1], TEST(int, 2), "*", TEST(int, 3));

    test_infix_expression(ce->args.data[2], TEST(int, 4), "+", TEST(int, 5));
}

void test_string_literal_expression(void) {
    char* input = "\"hello world\";";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_StringLiteral == es->expression.typ, "type not StringLiteral");
    StringLiteral* sl = es->expression.obj;
    if (strncmp("hello world", sl->tok.start, sl->tok.length)) {
        puts("wrong StringLiteral.Value");
        TEST_FAIL();
    }
}

void test_parsing_array_literals(void) {
    char* input = "[1, 2 * 2, 3 + 3]";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_ArrayLiteral == es->expression.typ, "type not ArrayLiteral");
    ArrayLiteral* al = es->expression.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, al->elements.length, "wrong length of ArrayLiteral.elements");

    test_integer_literal(al->elements.data[0], 1);
    test_infix_expression(al->elements.data[1],
            TEST(int, 2), "*", TEST(int, 2));
    test_infix_expression(al->elements.data[2],
            TEST(int, 3), "+", TEST(int, 3));
}

void test_parsing_index_expressions(void) {
    char* input = "myArray[1 + 1]";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_IndexExpression == es->expression.typ,
            "type not IndexExpression");
    IndexExpression* ie = es->expression.obj;

    test_identifier(ie->left, "myArray");
    test_infix_expression(ie->index, TEST(int, 1), "+", TEST(int, 1));
}

void test_parsing_table_literals_string_keys(void) {
    char* input = "{\"one\": 1, \"two\": 2, \"three\": 3}";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_TableLiteral == es->expression.typ,
            "type not TableLiteral");
    TableLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, hl->pairs.length, "wrong TableLiteral.pairs length");

    struct {
        char* key;
        long value;
    } expected[] = {
        {"one", 1},
        {"two", 2},
        {"three", 3},
    };
    int expected_len = sizeof(expected) / sizeof(expected[0]);
    for (int i = 0; i < expected_len; i++) {
        Pair* pair = &hl->pairs.data[i];
        TEST_ASSERT_NOT_NULL_MESSAGE(pair->val.obj, "value not found");
        test_integer_literal(pair->val, expected[i].value);
    }
}

void test_parsing_empty_table_literal(void) {
    char* input = "{}";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_TableLiteral == es->expression.typ,
            "type not TableLiteral");
    TableLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, hl->pairs.length, "wrong TableLiteral.pairs length");
}

void test_parsing_table_literals_with_expressions(void) {
    char* input = "{\"one\": 0 + 1, \"two\": 10 - 8, \"three\": 15 / 5}";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_TableLiteral == es->expression.typ,
            "type not TableLiteral");
    TableLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, hl->pairs.length, "wrong TableLiteral.pairs length");

    struct Test {
        char* key;
        long left;
        char* op;
        long right;
    } expected[] = {
        {"one", 0, "+", 1},
        {"two", 10, "-", 8},
        {"three", 15, "/", 5},
    };
    int expected_len = sizeof(expected) / sizeof(expected[0]);
    for (int i = 0; i < expected_len; i++) {
        struct Test test = expected[i];
        Pair* pair = &hl->pairs.data[i];
        StringLiteral *str = pair->key.obj;
        if (strncmp(test.key, str->tok.start, str->tok.length)) {
            printf("value %s not found got %.*s\n",
                    test.key, str->tok.length, str->tok.start);
            TEST_FAIL();
        }
        test_infix_expression(pair->val,
                TEST(int, test.left), test.op, TEST(int, test.right));
    }
}

void test_parsing_assign_expressions(void) {
    char* input = "foobar = 0;";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.stmts.length,
            "wrong prog.statements length");

    Node n = prog.stmts.data[0];

    TEST_ASSERT_MESSAGE(
            n_AssignStatement == n.typ, "type not AssignStatement");

    AssignStatement* as = n.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_Identifier, as->left.typ,
            "AssignStatement.Left is not Identifier");

    Identifier* ident = as->left.obj;

    if (strncmp("foobar", ident->tok.start, ident->tok.length)) {
        puts("wrong Identifier.Token.value");
        TEST_FAIL();
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IntegerLiteral, as->right.typ,
            "AssignStatement.Right is not IntegerLiteral");

    IntegerLiteral* int_lit = as->right.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, int_lit->value, "wrong IntegerLiteral.value");
}

void test_parsing_index_assign_expressions(void) {
    char* input = "foobar[12] = 69;";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.stmts.length,
            "wrong prog.statements length");

    Node n = prog.stmts.data[0];

    TEST_ASSERT_MESSAGE(
            n_AssignStatement == n.typ, "type not AssignStatement");

    AssignStatement* as = n.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IndexExpression, as->left.typ,
            "AssignStatement.Left is not IndexExpression");

    IndexExpression* index = as->left.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_Identifier, index->left.typ,
            "IndexExpression.Left is not Identifier");

    Identifier* ident = index->left.obj;

    if (strncmp("foobar", ident->tok.start, ident->tok.length)) {
        puts("wrong Identifier.Token.value");
        TEST_FAIL();
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IntegerLiteral, index->index.typ,
            "IndexExpression.Index is not IntegerLiteral");

    IntegerLiteral* index_int = index->index.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            12, index_int->value, "wrong IndexExpression.Index.value");

    IntegerLiteral* int_lit = as->right.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            69, int_lit->value, "wrong AssignStatement.Right.value");
}

void test_parsing_null_literals(void) {
    char* input = "null";

    parser_init(&p);
    prog = parse(&p, input);

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.stmts.length,
            "wrong prog.statements length");

    Node n = prog.stmts.data[0];

    TEST_ASSERT_MESSAGE(
            n_ExpressionStatement == n.typ, "type not ExpressionStatement");

    ExpressionStatement* es = n.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_NullLiteral, es->expression.typ,
            "ExpressionStatement Value is not NullLiteral");

    NullLiteral *nl = es->expression.obj;

    if (strncmp("null", nl->tok.start, nl->tok.length)) {
        puts("wrong NullLiteral.Token.value");
        TEST_FAIL();
    }
}

void test_parser_errors(void) {
    struct Test {
        const char* input;
        const char* error;
    } tests[] = {
        {"let x = =;", "unexpected token '='"},
        {"let = = 1;", "expected next token to be 'Identifier', got '=' instead"},
        {"return let;", "expected next token to be 'Identifier', got ';' instead"},

        {"1..5", "could not parse '1..5' as float"},
        {
            "15189704987123048718947",
            "could not parse '15189704987123048718947' as integer"
        },

        {"10 + + 5", "unexpected token '+'"},
        {"1 === 1", "unexpected token '='"},

        {"if ()", "empty if statement"},
        {"if (}", "unexpected token '}'"},
        {"if (true) }", "expected next token to be '{', got '}' instead"},
        {"if (true) {", "expected token to be '}', got 'Eof' instead"},
        {
            "if (x < y) { x } else if { y }",
            "expected next token to be '{', got 'if' instead"
        },

        {"fn (1)", "expected token to be 'Identifier', got 'Digit' instead"},
        {"add(4 5);", "expected next token to be ')', got 'Digit' instead"},

        {"\"hello world\"\";", "missing closing '\"' for string"},
        {"[1 2]", "expected next token to be ']', got 'Digit' instead"},
        {"[1, 1 2]", "expected next token to be ']', got 'Digit' instead"},

        {"array[1 2]", "expected next token to be ']', got 'Digit' instead"},

        {"{ return; }", "unexpected token 'return'"},
        {"{1, 3}", "expected next token to be ':', got ',' instead"},
        {"{1: 2, 3}", "expected next token to be ':', got '}' instead"},
        {"{:}", "unexpected token ':'"},
        {"{,}", "unexpected token ','"},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    bool pass = true;
    for (int i = 0; i < tests_len; ++i) {
        struct Test test = tests[i];
        prog = parse(&p, test.input);

        if (p.errors.length == 0) {
            printf("expected parser error for test: %s\n", test.input);
            pass = false;
        }

        if (test.error && strcmp(test.error, p.errors.data[0].message) != 0) {
            printf("wrong parser error for test: %s\nwant= %s\ngot = %s\n",
                    test.input, test.error, p.errors.data[0].message);
            pass = false;
        }

        __program_free();
        parser_free(&p);
        p.errors = (ErrorBuffer){0};
    }

    TEST_ASSERT(pass);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_let_statements);
    RUN_TEST(test_return_statements);
    RUN_TEST(test_identifier_expression);
    RUN_TEST(test_integer_literal_expression);
    RUN_TEST(test_float_literal_expression);
    RUN_TEST(test_parsing_prefix_expressions);
    RUN_TEST(test_parsing_infix_expressions);
    RUN_TEST(test_operator_precedence_parsing);
    RUN_TEST(test_boolean_expression);
    RUN_TEST(test_if_expression);
    RUN_TEST(test_if_else_expression);
    RUN_TEST(test_function_literal_parsing);
    RUN_TEST(test_function_literal_with_name);
    RUN_TEST(test_function_parameter_parsing);
    RUN_TEST(test_call_expression_parsing);
    RUN_TEST(test_string_literal_expression);
    RUN_TEST(test_parsing_array_literals);
    RUN_TEST(test_parsing_index_expressions);
    RUN_TEST(test_parsing_table_literals_string_keys);
    RUN_TEST(test_parsing_empty_table_literal);
    RUN_TEST(test_parsing_table_literals_with_expressions);
    RUN_TEST(test_parsing_assign_expressions);
    RUN_TEST(test_parsing_index_assign_expressions);
    RUN_TEST(test_parsing_null_literals);
    RUN_TEST(test_parser_errors);
    return UNITY_END();
}
