#include "unity/unity.h"

#include "tests.h"

#include "../src/ast.h"
#include "../src/parser.h"
#include "../src/token.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static void check_parser_errors(Parser* p) {
    if (p->errors.length == 0) {
        return;
    }

    printf("parser had %d errors\n", p->errors.length);
    for (int i = 0; i < p->errors.length; i++) {
        printf("%s\n", p->errors.data[i]);
    }
    TEST_FAIL();
}

static void test_let_statement(Node stmt, const char* exp_name) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(token_literal(stmt), "let",
            "wrong LetStatement.Token.literal");
    TEST_ASSERT_MESSAGE(n_LetStatement == stmt.typ, "type not LetStatement");

    LetStatement* let_stmt = (LetStatement*)stmt.obj;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(exp_name, let_stmt->name->tok.literal,
            "wrong LetStatement.Identifier.Value");
}

void test_return_statements(void) {
    char* input = "\
return 5;\
return 10;\
return 993322;\
";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, prog.stmts.length, "wrong prog.statements length");

    for (int i = 0; i < prog.stmts.length; i++) {
        Node stmt = prog.stmts.data[i];
        TEST_ASSERT_MESSAGE(n_ReturnStatement == stmt.typ,
                "type not ReturnStatement");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                "return", ((Token*)stmt.obj)->literal,
                "wrong ReturnStatement.Token.literal");
    }

    program_free(&prog);
    parser_free(&p);
}

void test_identifier_expression(void) {
    char* input = "foobar;";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "foobar", ident->tok.literal, "wrong Identifier.Token.value");

    program_free(&prog);
    parser_free(&p);
}

static void
_test_integer_literal(Node n, long value, char* expected) {
    TEST_ASSERT_MESSAGE(n_IntegerLiteral == n.typ, "type not IntegerLiteral");

    IntegerLiteral* il = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            value, il->value, "wrong IntegerLiteral.value");

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            expected, il->tok.literal, "wrong IntegerLiteral.tok.literal");
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

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            expected, il->tok.literal, "wrong FloatLiteral.tok.literal");
    free(expected);
}

void test_integer_literal_expression(void) {
    char* input = "5;";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "5", int_lit->tok.literal, "wrong IntegerLiteral.Token.literal");

    program_free(&prog);
    parser_free(&p);
}

void test_float_literal_expression(void) {
    char* input = "5.01;";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "5.01", fl_lit->tok.literal, "wrong FloatLiteral.Token.literal");

    program_free(&prog);
    parser_free(&p);
}

static void
test_identifier(Node n, const char* value) {
    TEST_ASSERT_MESSAGE(n_Identifier == n.typ, "type not type Identifier");
    Identifier* id = n.obj;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            value, id->tok.literal, "wrong Identifier.Value");
}

static void
test_boolean_literal(Node n, bool exp) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_BooleanLiteral, n.typ, "wrong type BooleanLiteral");
    BooleanLiteral* b = n.obj;
    TEST_ASSERT_MESSAGE(b->value == exp, "wrong BooleanLiteral.value");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            exp ? "true" : "false",
            b->tok.literal, "wrong BooleanLiteral.Token.literal");
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
    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Parser p;
        parser_init(&p);
        Program prog = parse(&p, test.input);
        TEST_ASSERT_NOT_NULL_MESSAGE(
                prog.stmts.data, "program.statements NULL");
        check_parser_errors(&p);

        Node stmt = prog.stmts.data[0];
        test_let_statement(stmt, test.expectedIdent);

        LetStatement* ls = stmt.obj;
        test_literal_expression(ls->value, test.expectedVal);

        program_free(&prog);
        parser_free(&p);
    }
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
    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Parser p;
        parser_init(&p);
        Program prog = parse(&p, test.input);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.operator, pe->op, "wrong PrefixExpression.op");

        test_literal_expression(pe->right, test.value);

        program_free(&prog);
        parser_free(&p);
    }
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
    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Parser p;
        parser_init(&p);
        Program prog = parse(&p, test.input);
        TEST_ASSERT_NOT_NULL_MESSAGE
            (prog.stmts.data, "program.statements NULL");

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

        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.operator, ie->op, "wrong InfixExpression.op");

        test_literal_expression(ie->right, test.right_value);

        program_free(&prog);
        parser_free(&p);
    }
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
    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Parser p;
        parser_init(&p);
        Program prog = parse(&p, test.input);
        TEST_ASSERT_NOT_NULL_MESSAGE
            (prog.stmts.data, "program.statements NULL");
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

        TEST_ASSERT_MESSAGE
            (program_fprint(&prog, fp) != -1, "program_fprint fail");
        fflush(fp);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.expected, buf, "program_fprint wrong");

        fclose(fp);
        free(buf);
        program_free(&prog);
        parser_free(&p);
    }
}

static void
test_infix_expression(Node n, Test *left, char* operator, Test *right) {
    TEST_ASSERT_MESSAGE
        (n_InfixExpression == n.typ, "type not type InfixExpression");

    InfixExpression* ie = n.obj;
    test_literal_expression(ie->left, left);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            operator, ie->op, "wrong InfixExpression.op");
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
    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Parser p;
        parser_init(&p);
        Program prog = parse(&p, test.input);
        TEST_ASSERT_NOT_NULL_MESSAGE
            (prog.stmts.data, "program.statements NULL");
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

        program_free(&prog);
        parser_free(&p);
    }
}

void test_if_expression(void) {
    char* input = "if (x < y) { x }";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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

    program_free(&prog);
    parser_free(&p);
}

void test_if_else_expression(void) {
    char* input = "if (x < y) { x } else { y }";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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

    program_free(&prog);
    parser_free(&p);
}

void test_function_literal_parsing(void) {
    char* input = "fn(x, y) { x + y; }";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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

    program_free(&prog);
    parser_free(&p);
}

void test_function_literal_with_name(void) {
    char* input = "let myFunction = fn() { };";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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

    if (strcmp("myFunction", fl->name) != 0) {
        printf("function literal name wrong. want 'myFunction', got='%s'\n",
                fl->name);
        TEST_FAIL();
    }

    program_free(&prog);
    parser_free(&p);
}

void test_function_parameter_parsing(void) {
    struct Test {
        char* input;
        char* expectedParams[3];
        int len;
    } tests[] = {
        {"fn() {};", {}, 0},
        {"fn(x) {};", {"x"}, 1},
        {"fn(x, y, z) {};", {"x", "y", "z"}, 3},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);
    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Parser p;
        parser_init(&p);
        Program prog = parse(&p, test.input);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
        check_parser_errors(&p);
        TEST_ASSERT_EQUAL_INT_MESSAGE
            (1, prog.stmts.length, "wrong prog.statements length");

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

        program_free(&prog);
        parser_free(&p);
    }
}

void test_call_expression_parsing(void) {
    char* input = "add(1, 2 * 3, 4 + 5);";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");

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

    program_free(&prog);
    parser_free(&p);
}

void test_string_literal_expression(void) {
    char* input = "\"hello world\";";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_StringLiteral == es->expression.typ, "type not StringLiteral");
    StringLiteral* sl = es->expression.obj;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "hello world", sl->tok.literal, "wrong StringLiteral.Value");

    program_free(&prog);
    parser_free(&p);
}

void test_parsing_array_literals(void) {
    char* input = "[1, 2 * 2, 3 + 3, 0xdeadbeef, 0b11011]";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_ArrayLiteral == es->expression.typ, "type not ArrayLiteral");
    ArrayLiteral* al = es->expression.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            5, al->elements.length, "wrong length of ArrayLiteral.elements");

    test_integer_literal(al->elements.data[0], 1);
    test_infix_expression(al->elements.data[1],
            TEST(int, 2), "*", TEST(int, 2));
    test_infix_expression(al->elements.data[2],
            TEST(int, 3), "+", TEST(int, 3));
    _test_integer_literal(al->elements.data[3], 0xdeadbeef, "0xdeadbeef");
    _test_integer_literal(al->elements.data[4], 0b11011, "0b11011");

    program_free(&prog);
    parser_free(&p);
}

void test_parsing_index_expressions(void) {
    char* input = "myArray[1 + 1]";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
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

    program_free(&prog);
    parser_free(&p);
}

void test_parsing_hash_literals_string_keys(void) {
    char* input = "{\"one\": 1, \"two\": 2, \"three\": 3}";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_HashLiteral == es->expression.typ,
            "type not HashLiteral");
    HashLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, hl->pairs.length, "wrong HashLiteral.pairs length");

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

    program_free(&prog);
    parser_free(&p);
}

void test_parsing_empty_hash_literal(void) {
    char* input = "{}";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_HashLiteral == es->expression.typ,
            "type not HashLiteral");
    HashLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, hl->pairs.length, "wrong HashLiteral.pairs length");

    program_free(&prog);
    parser_free(&p);
}

void test_parsing_hash_literals_with_expressions(void) {
    char* input = "{\"one\": 0 + 1, \"two\": 10 - 8, \"three\": 15 / 5}";

    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, prog.stmts.length, "wrong prog.statements length");

    ExpressionStatement* es = prog.stmts.data[0].obj;
    TEST_ASSERT_MESSAGE(
            n_HashLiteral == es->expression.typ,
            "type not HashLiteral");
    HashLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, hl->pairs.length, "wrong HashLiteral.pairs length");

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
        TEST_ASSERT_EQUAL_STRING_MESSAGE(test.key,
                ((StringLiteral*)pair->key.obj)->tok.literal, "value not found");
        test_infix_expression(pair->val,
                TEST(int, test.left), test.op, TEST(int, test.right));
    }

    program_free(&prog);
    parser_free(&p);
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
    RUN_TEST(test_parsing_hash_literals_string_keys);
    RUN_TEST(test_parsing_empty_hash_literal);
    RUN_TEST(test_parsing_hash_literals_with_expressions);
    return UNITY_END();
}
