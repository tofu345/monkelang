#include "unity/unity.h"
#include "ast.h"
#include "parser.h"
#include "lexer.h"
#include "token.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static void check_parser_errors(Parser* p) {
    if (p->errors_len == 0) {
        return;
    }

    printf("parser had %d errors\n", (int)p->errors_len);
    for (size_t i = 0; i < p->errors_len; i++) {
        printf("parser error: %s\n", p->errors[i]);
    }
    TEST_FAIL();
}

static void test_let_statement(Node stmt, const char* exp_name) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(token_literal(stmt), "let",
            "assert LetStatement.Token.Literal");
    TEST_ASSERT_EQUAL_INT_MESSAGE(n_LetStatement, stmt.typ,
            "assert type LetStatement");

    LetStatement* let_stmt = (LetStatement*)stmt.obj;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(exp_name, let_stmt->name->value,
            "assert LetStatement.Identifier.Value");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(exp_name, let_stmt->name->tok.literal,
            "assert LetStatement.Identifier.Token.Literal");
}

void test_return_statements(void) {
    char* input = "\
return 5;\
return 10;\
return 993322;\
";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, prog.len, "prog.statements length");

    for (size_t i = 0; i < prog.len; i++) {
        Node stmt = prog.stmts[i];
        TEST_ASSERT_EQUAL_INT_MESSAGE(n_ReturnStatement, stmt.typ,
                "stmt not ReturnStatement");
        TEST_ASSERT_EQUAL_STRING(
                "return", ((Token*)stmt.obj)->literal);
    }

    program_destroy(&prog);
    parser_destroy(p);
}

void test_identifier_expression(void) {
    char* input = "foobar;";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

    Node n = prog.stmts[0];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, n.typ, "assert ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_Identifier, es->expression.typ, "assert Identifier");

    Identifier* ident = es->expression.obj;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "foobar", ident->tok.literal, "assert Identifier.Token.Literal");

    program_destroy(&prog);
    parser_destroy(p);
}

static void
test_integer_literal(Node n, long value) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IntegerLiteral, n.typ, "assert IntegerLiteral");

    IntegerLiteral* il = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            value, il->value, "assert IntegerLiteral.value");

    char* expected = NULL;
    if (asprintf(&expected, "%ld", value) == -1)
        TEST_FAIL_MESSAGE("no memory");

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            expected, il->tok.literal, "assert IntegerLiteral.tok.literal");
    free(expected);
}

void test_integer_literal_expression(void) {
    char* input = "5;";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

    Node n = prog.stmts[0];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, n.typ, "assert ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IntegerLiteral, es->expression.typ, "assert IntegerLiteral");

    IntegerLiteral* int_lit = es->expression.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            5, int_lit->value, "assert IntegerLiteral.value");

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "5", int_lit->tok.literal, "IntegerLiteral.Token.Literal");

    program_destroy(&prog);
    parser_destroy(p);
}

static void
test_identifier(Node n, const char* value) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_Identifier, n.typ, "assert type Identifier");
    Identifier* id = n.obj;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            value, id->value, "assert Identifier.Value");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            value, id->tok.literal, "assert Identifier.Token.Literal");
}

static void
test_boolean_literal(Node n, bool exp) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_Boolean, n.typ, "assert type Boolean");
    Boolean* b = n.obj;
    TEST_ASSERT_MESSAGE(b->value == exp, "assert Boolean.value");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            exp ? "true" : "false",
            b->tok.literal, "assert Boolean.Token.literal");
}

typedef enum {
    l_Integer,
    l_String,
    l_Boolean,
} L_Type;

typedef union {
    long integer;
    const char* string;
    bool boolean;
} L_Test;

static void
test_literal_expression(Node n, L_Test exp, L_Type typ) {
    switch (typ) {
        case l_Integer:
            test_integer_literal(n, exp.integer);
            break;
        case l_String:
            test_identifier(n, exp.string);
            break;
        case l_Boolean:
            test_boolean_literal(n, exp.boolean);
            break;
        default:
            fprintf(stderr, "type of exp not handled. got %d", typ);
            exit(1);
    }
}

void test_let_statements(void) {
    struct Test {
        const char* input;
        const char* expectedIdent;
        L_Test expectedVal;
        L_Type t;
    } tests[] = {
        {"let x = 5;", "x", {5}, l_Integer},
        {"let y = true;", "y", {true}, l_Boolean},
        {"let foobar = y;", "foobar", (L_Test){ .string = "y" }, l_String},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");
        check_parser_errors(p);

        Node stmt = prog.stmts[0];
        test_let_statement(stmt, test.expectedIdent);

        LetStatement* ls = stmt.obj;
        test_literal_expression(ls->value, test.expectedVal, test.t);

        program_destroy(&prog);
        parser_destroy(p);
    }
}

void test_parsing_prefix_expressions(void) {
    struct Test {
        char* input;
        char* operator;
        L_Test value;
        L_Type t;
    } tests[] = {
        {"!5;", "!", {5}, l_Integer},
        {"-15;", "-", {15}, l_Integer},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

        check_parser_errors(p);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

        Node n = prog.stmts[0];
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_ExpressionStatement, n.typ, "assert ExpressionStatement");

        ExpressionStatement* es = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_PrefixExpression, es->expression.typ, "assert PrefixExpression");

        PrefixExpression* pe = es->expression.obj;
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.operator, pe->op, "assert PrefixExpression.op");

        test_literal_expression(pe->right, test.value, test.t);

        program_destroy(&prog);
        parser_destroy(p);
    }
}

void test_parsing_infix_expressions(void) {
    struct Test {
        char* input;
        L_Test left_value;
        char* operator;
        L_Test right_value;
        L_Type t;
    } tests[] = {
        {"5 + 5;", {5}, "+", {5}, l_Integer},
        {"5 - 5;", {5}, "-", {5}, l_Integer},
        {"5 * 5;", {5}, "*", {5}, l_Integer},
        {"5 / 5;", {5}, "/", {5}, l_Integer},
        {"5 > 5;", {5}, ">", {5}, l_Integer},
        {"5 < 5;", {5}, "<", {5}, l_Integer},
        {"5 == 5;", {5}, "==", {5}, l_Integer},
        {"5 != 5;", {5}, "!=", {5}, l_Integer},
        {"true == true", {true}, "==", {true}, l_Boolean},
        {"true != false", {true}, "!=", {false}, l_Boolean},
        {"false == false", {false}, "==", {false}, l_Boolean},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

        check_parser_errors(p);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

        Node n = prog.stmts[0];
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_ExpressionStatement, n.typ, "assert ExpressionStatement");

        ExpressionStatement* es = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_InfixExpression, es->expression.typ, "assert InfixExpression");

        InfixExpression* ie = es->expression.obj;

        test_literal_expression(ie->left, test.left_value, test.t);

        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.operator, ie->op, "assert InfixExpression.op");

        test_literal_expression(ie->right, test.right_value, test.t);

        program_destroy(&prog);
        parser_destroy(p);
    }
}

void test_operator_precedence_parsing(void) {
    struct Test {
        char* input;
        char* expected;
    } tests[] = {
        {
            "-1 * 2 + 3",
            "(((-1) * 2) + 3)",
        },
        {
            "-a * b",
            "((-a) * b)",
        },
        {
            "!-a",
            "(!(-a))",
        },
        {
            "a + b + c",
            "((a + b) + c)",
        },
        {
            "a + b - c",
            "((a + b) - c)",
        },
        {
            "a * b * c",
            "((a * b) * c)",
        },
        {
            "a * b / c",
            "((a * b) / c)",
        },
        {
            "a + b / c",
            "(a + (b / c))",
        },
        {
            "a + b * c + d / e - f",
            "(((a + (b * c)) + (d / e)) - f)",
        },
        {
            "3 + 4; -5 * 5",
            "(3 + 4)((-5) * 5)",
        },
        {
            "5 > 4 == 3 < 4",
            "((5 > 4) == (3 < 4))",
        },
        {
            "5 < 4 != 3 > 4",
            "((5 < 4) != (3 > 4))",
        },
        {
            "3 + 4 * 5 == 3 * 1 + 4 * 5",
            "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))",
        },
        {
            "true",
            "true",
        },
        {
            "false",
            "false",
        },
        {
            "3 > 5 == false",
            "((3 > 5) == false)",
        },
        {
            "3 < 5 == true",
            "((3 < 5) == true)",
        },
        {
            "1 + (2 + 3) + 4",
            "((1 + (2 + 3)) + 4)",
        },
        {
            "(5 + 5) * 2",
            "((5 + 5) * 2)",
        },
        {
            "2 / (5 + 5)",
            "(2 / (5 + 5))",
        },
        {
            "-(5 + 5)",
            "(-(5 + 5))",
        },
        {
            "!(true == true)",
            "(!(true == true))",
        },
        {
            "a + add(b * c) + d",
            "((a + add((b * c))) + d)",
        },
        {
            "add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 * 8))",
            "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))",
        },
        {
            "add(a + b + c * d / f + g)",
            "add((((a + b) + ((c * d) / f)) + g))",
        },
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");
        check_parser_errors(p);

        Node n = prog.stmts[0];
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_ExpressionStatement, n.typ, "assert ExpressionStatement");

        size_t len = strlen(test.expected) + 2;
        char* buf = calloc(len, sizeof(char));
        FILE* fp = fmemopen(buf, len, "w");
        if (fp == NULL) {
            fprintf(stderr, "no memory");
            exit(1);
        }

        TEST_ASSERT_MESSAGE(program_fprint(&prog, fp) != -1, "program_fprint fail");
        fflush(fp);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.expected, buf, "program_fprint wrong");

        fclose(fp);
        free(buf);
        program_destroy(&prog);
        parser_destroy(p);
    }
}

static void
test_infix_expression(Node n, L_Test left, char* operator, L_Test right,
        L_Type typ) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_InfixExpression, n.typ, "assert type InfixExpression");

    InfixExpression* ie = n.obj;
    test_literal_expression(ie->left, left, typ);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            operator, ie->op, "assert InfixExpression.op");
    test_literal_expression(ie->right, right, typ);
}

void test_boolean_expression(void) {
    struct Test {
        char* input;
        bool expected;
    } tests[] = {
        {"true;", true},
        {"false;", false},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");
        check_parser_errors(p);

        TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

        Node n = prog.stmts[0];
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_ExpressionStatement, n.typ, "assert ExpressionStatement");

        ExpressionStatement* es = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_Boolean, es->expression.typ, "assert Boolean type");

        Boolean* b = es->expression.obj;
        TEST_ASSERT_MESSAGE(
                b->value == test.expected, "assert Boolean.value");

        program_destroy(&prog);
        parser_destroy(p);
    }
}

void test_if_expression(void) {
    char* input = "if (x < y) { x }";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

    Node n = prog.stmts[0];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, n.typ, "assert ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IfExpression, es->expression.typ, "assert IfExpression");

    IfExpression* ie = es->expression.obj;
    test_infix_expression(ie->condition, (L_Test){ .string = "x" }, "<",
            (L_Test){ .string = "y" }, l_String);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ie->consequence->len,
            "IfExpression.consequence length");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, ie->consequence->statements[0].typ,
            "assert IfExpression.consequence.statements[0] ExpressionStatement");

    ExpressionStatement* consequence = ie->consequence->statements[0].obj;
    test_identifier(consequence->expression, "x");

    TEST_ASSERT_NULL_MESSAGE(ie->alternative,
            "assert IfExpression.alternative NULL");

    program_destroy(&prog);
    parser_destroy(p);
}

void test_if_else_expression(void) {
    char* input = "if (x < y) { x } else { y }";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

    Node n = prog.stmts[0];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, n.typ, "assert ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IfExpression, es->expression.typ, "assert IfExpression");

    IfExpression* ie = es->expression.obj;
    test_infix_expression(ie->condition, (L_Test){ .string = "x" }, "<",
            (L_Test){ .string = "y" }, l_String);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ie->consequence->len,
            "IfExpression.consequence length");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, ie->consequence->statements[0].typ,
            "assert IfExpression.consequence.statements[0] ExpressionStatement");

    ExpressionStatement* consequence = ie->consequence->statements[0].obj;
    test_identifier(consequence->expression, "x");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ie->alternative->len,
            "IfExpression.alternative length");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, ie->alternative->statements[0].typ,
            "assert IfExpression.alternative.statements[0] ExpressionStatement");

    ExpressionStatement* alternative = ie->alternative->statements[0].obj;
    test_identifier(alternative->expression, "y");

    program_destroy(&prog);
    parser_destroy(p);
}

void test_function_literal_parsing(void) {
    char* input = "fn(x, y) { x + y; }";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

    Node n = prog.stmts[0];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, n.typ, "assert ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_FunctionLiteral, es->expression.typ, "assert n_FunctionLiteral");

    FunctionLiteral* fl = es->expression.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            2, fl->params_len, "assert FunctionLiteral.paremeters_len");

    test_literal_expression((Node){ n_Identifier, fl->params[0] },
            (L_Test){ .string = "x" }, l_String);
    test_literal_expression((Node){ n_Identifier, fl->params[1] },
            (L_Test){ .string = "y" }, l_String);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, fl->body->len,
            "assert FunctionLiteral.body.len");

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, fl->body->statements[0].typ, "assert ExpressionStatement");

    ExpressionStatement* body_stmt = fl->body->statements[0].obj;

    test_infix_expression(body_stmt->expression,
            (L_Test){ .string = "x" }, "+", (L_Test){ .string = "y" },
            l_String);

    program_destroy(&prog);
    parser_destroy(p);
}

void test_function_parameter_parsing(void) {
    struct Test {
        const char* input;
        const char* expectedParams[3];
        int len;
    } tests[] = {
        {"fn() {};", {}, 0},
        {"fn(x) {};", {"x"}, 1},
        {"fn(x, y, z) {};", {"x", "y", "z"}, 3},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");
        check_parser_errors(p);

        // segfaults waiting to happen
        ExpressionStatement* es = prog.stmts[0].obj;
        FunctionLiteral* fl = es->expression.obj;

        TEST_ASSERT_EQUAL_INT_MESSAGE(
                test.len, fl->params_len,
                "assert FunctionLiteral.paremeters_len");

        for (int i = 0; i < test.len; i++) {
            test_literal_expression((Node){ n_Identifier, fl->params[i] },
                    (L_Test){ .string = test.expectedParams[i] }, l_String);
        }

        program_destroy(&prog);
        parser_destroy(p);
    }
}

void test_call_expression_parsing(void) {
    char* input = "add(1, 2 * 3, 4 + 5);";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts, "program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

    Node n = prog.stmts[0];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, n.typ, "assert ExpressionStatement");

    ExpressionStatement* es = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_CallExpression, es->expression.typ, "assert n_CallExpression");

    CallExpression* ce = es->expression.obj;

    test_identifier(ce->function, "add");

    TEST_ASSERT_EQUAL_INT_MESSAGE(
            3, ce->args_len, "assert CallExpression.args_len");

    test_literal_expression(ce->args[0],
            (L_Test){ .integer = 1 }, l_Integer);

    test_infix_expression(ce->args[1],
            (L_Test){ .integer = 2 }, "*", (L_Test){ .integer = 3 },
            l_Integer);

    test_infix_expression(ce->args[2],
            (L_Test){ .integer = 4 }, "+", (L_Test){ .integer = 5 },
            l_Integer);

    program_destroy(&prog);
    parser_destroy(p);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_let_statements);
    RUN_TEST(test_return_statements);
    RUN_TEST(test_identifier_expression);
    RUN_TEST(test_integer_literal_expression);
    RUN_TEST(test_parsing_prefix_expressions);
    RUN_TEST(test_parsing_infix_expressions);
    RUN_TEST(test_operator_precedence_parsing);
    RUN_TEST(test_boolean_expression);
    RUN_TEST(test_if_expression);
    RUN_TEST(test_if_else_expression);
    RUN_TEST(test_function_literal_parsing);
    RUN_TEST(test_function_parameter_parsing);
    RUN_TEST(test_call_expression_parsing);
    return UNITY_END();
}
