#include "unity/unity.h"
#include "helpers.h"

#include "../src/ast.h"
#include "../src/parser.h"
#include "../src/token.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_NODE_TYPE(t, n) \
    TEST_ASSERT_EQUAL_INT_MESSAGE(t, n.typ, "type not " #t);

static bool
test_token_literal(Token *tok, const char *expected) {
    // FIX? use strlen(expected)
    return strncmp(expected, tok->start, tok->length) == 0;
}

static void
test_integer_literal(Node n, long value) {
    ASSERT_NODE_TYPE(n_IntegerLiteral, n);

    Token *tok = node_token(n);
    char* exp = NULL;
    if (asprintf(&exp, "%ld", value) == -1) { die("vasprintf"); }
    bool eq = test_token_literal(tok, exp);
    free(exp);

    if (!eq) {
        printf("wrong IntegerLiteral.literal want %ld, got %.*s\n",
                value, tok->length, tok->start);
        TEST_FAIL();
    }

    IntegerLiteral* il = n.obj;
    if (il->value != value) {
        printf("wrong IntegerLiteral.value want %ld, got %ld\n",
                value, il->value);
        TEST_FAIL();
    }
}

static void
test_float_literal(Node n, double value) {
    ASSERT_NODE_TYPE(n_FloatLiteral, n);

    Token *tok = node_token(n);
    char* exp = NULL;
    if (asprintf(&exp, "%f", value) == -1) { die("vasprintf"); }
    bool eq = test_token_literal(tok, exp);
    free(exp);

    if (!eq) {
        printf("wrong FloatLiteral.literal want %f, got %.*s\n",
                value, tok->length, tok->start);
        TEST_FAIL();
    }

    FloatLiteral* lit = n.obj;
    if (lit->value != value) {
        printf("wrong FloatLiteral.value want %f, got %f\n",
                value, lit->value);
        TEST_FAIL();
    }
}

static void
test_identifier(Node n, const char* value) {
    ASSERT_NODE_TYPE(n_Identifier, n);

    Token *tok = node_token(n);
    if (!test_token_literal(tok, value)) {
        printf("wrong Identifier.literal want %s, got %.*s\n",
                value, tok->length, tok->start);
        TEST_FAIL();
    }
}

static void
test_boolean_literal(Node n, bool exp) {
    ASSERT_NODE_TYPE(n_BooleanLiteral, n);

    BooleanLiteral* b = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            b->value, exp, "wrong BooleanLiteral.value");

    const char *exp_lit = exp ? "true" : "false";
    if (!test_token_literal(&b->tok, exp_lit)) {
        printf("wrong BooleanLiteral.Token.literal want %s, got %.*s\n",
                exp_lit, LITERAL(b->tok));
        TEST_FAIL();
    }
}

static void
test_node(Node n, Test *test) {
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
        case test_null:
            TEST_ASSERT_NULL_MESSAGE(n.obj, "Node not NULL");
            break;
        default:
            die("test_literal: type %d not handled", test->typ);
    }
}

static void
test_infix_expression(Node n, Test *left, char* operator, Test *right) {
    ASSERT_NODE_TYPE(n_InfixExpression, n);

    InfixExpression* ie = n.obj;
    test_node(ie->left, left);
    test_node(ie->right, right);

    if (!test_token_literal(&ie->tok, operator)) {
        printf("wrong InfixExpression Operator want %s, got %.*s\n",
                operator, LITERAL(ie->tok));
        TEST_FAIL();
    }
}

static void
test_let_statement(Node n, const char* exp_name) {
    ASSERT_NODE_TYPE(n_LetStatement, n);

    Token *tok = node_token(n);
    if (!test_token_literal(tok, "let")) {
        printf("wrong LetStatement.Token.literal want 'let' got %.*s\n",
                tok->length, tok->start);
        TEST_FAIL();
    }

    LetStatement *stmt = n.obj;
    tok = &stmt->name->tok;
    if (!test_token_literal(tok, exp_name)) {
        printf("wrong LetStatement.Identifier.Token.literal want %s, got %.*s\n",
                exp_name, tok->length, tok->start);
        TEST_FAIL();
    }
}

static void
check(Parser *p, Program *prog) {
    if (p->errors.length > 0) {
        print_parser_errors(p);
        TEST_FAIL();
    }

    TEST_ASSERT_MESSAGE(prog->stmts.length == 1, "wrong prog.statements length");
}

Parser p;
Program prog;

// free and reset [prog]
static void
_program_free(void) {
    program_free(&prog);
    prog = (Program){0};
}

void setUp(void) {}

void tearDown(void) {
    _program_free();
    parser_free(&p);
    p.errors = (ErrorBuffer){0};
}

void test_identifier_expression(void) {
    char *input = "foobar;";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);

    ExpressionStatement* es = n.obj;
    test_identifier(es->expression, "foobar");
}

void test_integer_literal_expression(void) {
    char *input = "5;";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    test_integer_literal(es->expression, 5);
}

void test_float_literal_expression(void) {
    char *input = "5.01;";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    test_float_literal(es->expression, 5.01);
}

void test_return_statements(void) {
    struct Test {
        const char *input;
        Test *expectedVal;
    } tests[] = {
        {"return;", TEST_NULL},
        {"return 5;", TEST(int, 5)},
        {"return 10;", TEST(int, 10)},
        {"return 993322;", TEST(int, 993322)},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        prog = parse(&p, test.input);
        check(&p, &prog);

        Node stmt = prog.stmts.data[0];
        ASSERT_NODE_TYPE(n_ReturnStatement, stmt);

        if (!test_token_literal(node_token(stmt), "return")) {
            puts("wrong ReturnStatement.Token.literal");
            TEST_FAIL();
        }

        ReturnStatement *rs = stmt.obj;
        test_node(rs->return_value, test.expectedVal);

        _program_free();
    }
}

void test_let_statements(void) {
    struct Test {
        const char *input;
        const char *expectedIdent;
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
        check(&p, &prog);

        Node stmt = prog.stmts.data[0];
        test_let_statement(stmt, test.expectedIdent);

        LetStatement* ls = stmt.obj;
        test_node(ls->value, test.expectedVal);

        _program_free();
    }
}

void test_parsing_prefix_expressions(void) {
    struct Test {
        char *input;
        char *operator;
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
        check(&p, &prog);

        Node n = prog.stmts.data[0];
        ASSERT_NODE_TYPE(n_ExpressionStatement, n);
        ExpressionStatement* es = n.obj;

        ASSERT_NODE_TYPE(n_PrefixExpression, es->expression);
        PrefixExpression* pe = es->expression.obj;

        test_node(pe->right, test.value);

        if (!test_token_literal(&pe->tok, test.operator)) {
            puts("wrong PrefixExpression Operator");
            TEST_FAIL();
        }

        _program_free();
    }
}

void test_parsing_infix_expressions(void) {
    struct Test {
        char *input;
        Test *left_value;
        char *operator;
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
        check(&p, &prog);

        Node n = prog.stmts.data[0];
        ASSERT_NODE_TYPE(n_ExpressionStatement, n);
        ExpressionStatement* es = n.obj;

        ASSERT_NODE_TYPE(n_InfixExpression, es->expression);
        test_infix_expression(es->expression, test.left_value, test.operator,
                              test.right_value);

        _program_free();
    }
}

void test_operator_precedence_parsing(void) {
    struct Test {
        char *input;
        char *expected;
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

        // not check() because test: "3 + 4; -5 * 5" has more than one statement
        if (p.errors.length > 0) {
            print_parser_errors(&p);
            TEST_FAIL();
        }

        for (int i = 0; i < prog.stmts.length; ++i) {
            ASSERT_NODE_TYPE(n_ExpressionStatement, prog.stmts.data[i]);
        }

        char *buf = NULL;
        size_t len;
        FILE *fp = open_memstream(&buf, &len);
        TEST_ASSERT_NOT_NULL_MESSAGE(fp, "open_memstream fail");

        TEST_ASSERT_MESSAGE(
                program_fprint(&prog, fp) != -1, "program_fprint fail");

        fclose(fp);

        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.expected, buf, "program_fprint wrong");

        free(buf);

        _program_free();
    }
}

void test_boolean_expression(void) {
    struct Test {
        char *input;
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

        check(&p, &prog);

        Node n = prog.stmts.data[0];
        ASSERT_NODE_TYPE(n_ExpressionStatement, n);
        ExpressionStatement* es = n.obj;

        ASSERT_NODE_TYPE(n_BooleanLiteral, es->expression);
        BooleanLiteral* b = es->expression.obj;

        TEST_ASSERT_MESSAGE(
                b->value == test.expected, "wrong BooleanLiteral.value");

        _program_free();
    }
}

void test_if_expression(void) {
    char *input = "if (x < y) { x }";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_IfExpression, es->expression);
    IfExpression* ie = es->expression.obj;

    test_infix_expression(ie->condition, TEST(str, "x"), "<", TEST(str, "y"));

    NodeBuffer consequence = ie->consequence->stmts;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, consequence.length, "wrong Consequence length");

    ASSERT_NODE_TYPE(n_ExpressionStatement, consequence.data[0]);
    ExpressionStatement* stmt = consequence.data[0].obj;
    test_identifier(stmt->expression, "x");

    TEST_ASSERT_NULL_MESSAGE(ie->alternative, "Alternative is not NULL");
}

void test_if_else_expression(void) {
    char *input = "if (x < y) { x } else { y }";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_IfExpression, es->expression);
    IfExpression* ie = es->expression.obj;
    test_infix_expression(ie->condition, TEST(str, "x"), "<", TEST(str, "y"));

    NodeBuffer consequence = ie->consequence->stmts;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, consequence.length, "wrong Consequence length");

    ASSERT_NODE_TYPE(n_ExpressionStatement, consequence.data[0]);
    ExpressionStatement* stmt = consequence.data[0].obj;
    test_identifier(stmt->expression, "x");

    NodeBuffer alternative = ie->alternative->stmts;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, alternative.length, "wrong Alternative length");

    ASSERT_NODE_TYPE(n_ExpressionStatement, alternative.data[0]);
    stmt = ie->alternative->stmts.data[0].obj;
    test_identifier(stmt->expression, "y");
}

void test_function_literal_parsing(void) {
    char *input = "fn(x, y) { x + y; }";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_FunctionLiteral, es->expression);
    FunctionLiteral* fl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            2, fl->params.length, "wrong FunctionLiteral.parems.length");

    test_identifier(NODE(n_Identifier, fl->params.data[0]), "x");
    test_identifier(NODE(n_Identifier, fl->params.data[1]), "y");

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, fl->body->stmts.length, "wrong FunctionLiteral.body.len");

    ASSERT_NODE_TYPE(n_ExpressionStatement, fl->body->stmts.data[0]);

    ExpressionStatement *stmt = fl->body->stmts.data[0].obj;

    test_infix_expression(stmt->expression, TEST(str, "x"), "+", TEST(str, "y"));
}

void test_function_literal_with_name(void) {
    char *input = "let myFunction = fn() { };";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_LetStatement, n);
    LetStatement* ls = n.obj;

    ASSERT_NODE_TYPE(n_FunctionLiteral, ls->value);
    FunctionLiteral* fl = ls->value.obj;
    Token *tok = &fl->name->tok;
    if (!test_token_literal(tok, "myFunction")) {
        printf("function literal name wrong. want 'myFunction', got='%.*s'\n",
                tok->length, tok->start);
        TEST_FAIL();
    }
}

void test_function_parameter_parsing(void) {
    struct Test {
        char *input;
        char *expectedParams[3];
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
        check(&p, &prog);

        Node n = prog.stmts.data[0];
        ASSERT_NODE_TYPE(n_ExpressionStatement, n);
        ExpressionStatement* es = n.obj;

        ASSERT_NODE_TYPE(n_FunctionLiteral, es->expression);
        FunctionLiteral* fl = es->expression.obj;

        TEST_ASSERT_EQUAL_INT_MESSAGE(
                test.len, fl->params.length,
                "wrong FunctionLiteral.paremeters_len");

        for (int i = 0; i < test.len; i++) {
            test_identifier(
		    NODE(n_Identifier, fl->params.data[i]),
                    test.expectedParams[i]);
        }

        _program_free();
    }
}

void test_call_expression_parsing(void) {
    char *input = "add(1, 2 * 3, 4 + 5);";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_CallExpression, es->expression);
    CallExpression* ce = es->expression.obj;

    test_identifier(ce->function, "add");

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, ce->args.length, "wrong CallExpression.args_len");

    test_integer_literal(ce->args.data[0], 1);

    test_infix_expression(ce->args.data[1], TEST(int, 2), "*", TEST(int, 3));

    test_infix_expression(ce->args.data[2], TEST(int, 4), "+", TEST(int, 5));
}

void test_string_literal_expression(void) {
    char *input = "\"hello world\";";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_StringLiteral, es->expression);
    StringLiteral* sl = es->expression.obj;
    if (!test_token_literal(&sl->tok, "hello world")) {
        puts("wrong StringLiteral.Value");
        TEST_FAIL();
    }
}

void test_parsing_array_literals(void) {
    char *input = "[1, 2 * 2, 3 + 3]";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_ArrayLiteral, es->expression);
    ArrayLiteral* al = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, al->elements.length, "wrong length of ArrayLiteral.elements");

    test_integer_literal(al->elements.data[0], 1);
    test_infix_expression(al->elements.data[1], TEST(int, 2), "*", TEST(int, 2));
    test_infix_expression(al->elements.data[2], TEST(int, 3), "+", TEST(int, 3));
}

void test_parsing_index_expressions(void) {
    char *input = "myArray[1 + 1]";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_IndexExpression, es->expression);
    IndexExpression* ie = es->expression.obj;

    test_identifier(ie->left, "myArray");
    test_infix_expression(ie->index, TEST(int, 1), "+", TEST(int, 1));
}

void test_parsing_table_literals_string_keys(void) {
    char *input = "{\"one\": 1, \"two\": 2, \"three\": 3}";

    parser_init(&p);
    prog = parse(&p, input);

    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_TableLiteral, es->expression);
    TableLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, hl->pairs.length, "wrong TableLiteral.pairs length");

    struct {
        char *key;
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
    char *input = "{}";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_TableLiteral, es->expression);
    TableLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, hl->pairs.length, "wrong TableLiteral.pairs length");
}

void test_parsing_table_literals_with_expressions(void) {
    char *input = "{\"one\": 0 + 1, \"two\": 10 - 8, \"three\": 15 / 5}";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_TableLiteral, es->expression);
    TableLiteral* hl = es->expression.obj;

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, hl->pairs.length, "wrong TableLiteral.pairs length");

    struct Test {
        char *key;
        long left;
        char *op;
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
        if (!test_token_literal(&str->tok, test.key)) {
            printf("value %s not found got %.*s\n",
                    test.key, str->tok.length, str->tok.start);
            TEST_FAIL();
        }

        test_infix_expression(pair->val,
                TEST(int, test.left), test.op, TEST(int, test.right));
    }
}

void test_parsing_assignment(void) {
    char *input = "foobar = 0;";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_Assignment, n);
    Assignment* as = n.obj;

    ASSERT_NODE_TYPE(n_Identifier, as->left);

    Identifier* ident = as->left.obj;

    if (!test_token_literal(&ident->tok, "foobar")) {
        puts("wrong Identifier.Token.value");
        TEST_FAIL();
    }

    ASSERT_NODE_TYPE(n_IntegerLiteral, as->right);

    IntegerLiteral* int_lit = as->right.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, int_lit->value, "wrong IntegerLiteral.value");
}

void test_parsing_index_assignment(void) {
    char *input = "foobar[12] = 69;";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_Assignment, n);
    Assignment* as = n.obj;

    ASSERT_NODE_TYPE(n_IndexExpression, as->left);
    IndexExpression* ie = as->left.obj;

    ASSERT_NODE_TYPE(n_Identifier, ie->left);
    Identifier* ident = ie->left.obj;

    if (strncmp("foobar", ident->tok.start, ident->tok.length)) {
        puts("wrong Identifier.Token.value");
        TEST_FAIL();
    }

    ASSERT_NODE_TYPE(n_IntegerLiteral, ie->index);
    IntegerLiteral* index_int = ie->index.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            12, index_int->value, "wrong IndexExpression.Index.value");

    ASSERT_NODE_TYPE(n_IntegerLiteral, as->right);
    IntegerLiteral* int_lit = as->right.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            69, int_lit->value, "wrong AssignStatement.Right.value");
}

void test_parsing_operator_assignment(void) {
    struct Test {
        const char *input;
        const char *ident;
        const char *operator;
        Test *value;
    } tests[] = {
        {"a += 1", "a", "+", TEST(int, 1)},
        {"a -= 1", "a", "-", TEST(int, 1)},
        {"a *= 1", "a", "*", TEST(int, 1)},
        {"a /= 1", "a", "/", TEST(int, 1)},
    };
    int tests_len = sizeof(tests) / sizeof(tests[0]);

    parser_init(&p);

    for (int i = 0; i < tests_len; i++) {
        struct Test test = tests[i];
        prog = parse(&p, test.input);
        check(&p, &prog);

        Node n = prog.stmts.data[0];
        ASSERT_NODE_TYPE(n_OperatorAssignment, n);
        OperatorAssignment *stmt = n.obj;

        test_identifier(stmt->left, test.ident);

        TEST_ASSERT_EQUAL_STRING_LEN(
                test.operator, stmt->tok.start, strlen(test.operator));

        test_node(stmt->right, test.value);

        _program_free();
    }
}

void test_parsing_null_literals(void) {
    char *input = "null";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, n);
    ExpressionStatement* es = n.obj;

    ASSERT_NODE_TYPE(n_NullLiteral, es->expression);
    NullLiteral *nl = es->expression.obj;

    if (!test_token_literal(&nl->tok, "null")) {
        puts("wrong NullLiteral.Token.value");
        TEST_FAIL();
    }
}

void test_for_statement(void) {
    char *input = "for (let i = 0; i < 5; i = i + 1) { i == 10; }";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ForStatement, n);
    ForStatement* fs = n.obj;

    LetStatement* ls = fs->init.obj;
    test_let_statement(fs->init, "i");
    test_node(ls->value, TEST(int, 0));

    test_infix_expression(fs->condition, TEST(str, "i"), "<", TEST(int, 5));

    ASSERT_NODE_TYPE(n_Assignment, fs->update);
    Assignment* as = fs->update.obj;
    test_identifier(as->left, "i");
    test_infix_expression(as->right, TEST(str, "i"), "+", TEST(int, 1));

    BlockStatement *bs = fs->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, bs->stmts.length, "wrong BlockStatement length");

    Node stmt = bs->stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, stmt);

    ExpressionStatement* es = stmt.obj;
    test_infix_expression(es->expression, TEST(str, "i"), "==", TEST(int, 10));
}

void test_empty_for_statement(void) {
    char *input = "for (;;) { i == 10; }";

    parser_init(&p);
    prog = parse(&p, input);
    check(&p, &prog);

    Node n = prog.stmts.data[0];
    ASSERT_NODE_TYPE(n_ForStatement, n);
    ForStatement* fs = n.obj;

    test_node(fs->init, TEST_NULL);
    test_node(fs->condition, TEST_NULL);
    test_node(fs->update, TEST_NULL);

    BlockStatement *bs = fs->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, bs->stmts.length, "wrong BlockStatement length");

    Node stmt = bs->stmts.data[0];
    ASSERT_NODE_TYPE(n_ExpressionStatement, stmt);

    ExpressionStatement* es = stmt.obj;
    test_infix_expression(es->expression, TEST(str, "i"), "==", TEST(int, 10));
}

void test_parser_errors(void) {
    struct Test {
        const char *input;
        const char *error;
    } tests[] = {
        {"let x = =;", "unexpected token '='"},
        {"let = = 1;", "expected next token to be 'Identifier', got '=' instead"},
        {
            "return let;", // empty return statement followed by invalid let
                           // statement
            "expected next token to be 'Identifier', got ';' instead"
        },

        {"1..5", "could not parse '1..5' as float"},
        {
            "15189704987123048718947",
            "integer '15189704987123048718947' is out of range"
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

        {"fn (1)", "expected token to be 'Identifier', got 'Integer' instead"},
        {"add(4 5);", "expected next token to be ')', got 'Integer' instead"},
        {"add(return)", "unexpected token 'return'"},

        {"\"hello world\"\";", "missing closing '\"'"},
        {"[1 2]", "expected next token to be ']', got 'Integer' instead"},
        {"[1, 1 2]", "expected next token to be ']', got 'Integer' instead"},
        {"[ return; ]", "unexpected token 'return'"},

        {"array[1 2]", "expected next token to be ']', got 'Integer' instead"},

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

        } else if (strcmp(test.error, p.errors.data[0].message) != 0) {
            printf("wrong parser error for test: %s\nwant= %s\ngot = %s\n",
                    test.input, test.error, p.errors.data[0].message);
            pass = false;
        }

        _program_free();
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
    RUN_TEST(test_parsing_assignment);
    RUN_TEST(test_parsing_index_assignment);
    RUN_TEST(test_parsing_operator_assignment);
    RUN_TEST(test_parsing_null_literals);
    RUN_TEST(test_for_statement);
    RUN_TEST(test_empty_for_statement);
    RUN_TEST(test_parser_errors);
    return UNITY_END();
}
