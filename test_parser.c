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

static void test_let_statement(Node stmt, char* exp_name) {
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

void test_let_statements(void) {
    char* input = "\
let x = 5;\
let y = 10;\
let foobar = 838383;\
";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    if (prog.statements == NULL)
        TEST_FAIL_MESSAGE("program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, prog.len, "prog.statements length");

    char* tests[] = {
        "x",
        "y",
        "foobar",
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < tests_len; i++) {
        Node stmt = prog.statements[i];
        test_let_statement(stmt, tests[i]);
    }

    program_destroy(&prog);
    parser_destroy(p);
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
    if (prog.statements == NULL)
        TEST_FAIL_MESSAGE("program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, prog.len, "prog.statements length");

    for (size_t i = 0; i < prog.len; i++) {
        Node stmt = prog.statements[i];
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
    if (prog.statements == NULL)
        TEST_FAIL_MESSAGE("program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

    Node n = prog.statements[0];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, n.typ, "assert ExpressionStatement");

    ExpressionStatement* st = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_Identifier, st->expression.typ, "assert Identifier");

    Identifier* ident = st->expression.obj;
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
}

void test_integer_literal_expression(void) {
    char* input = "5;";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser* p = parser_new(&l);
    Program prog = parse_program(p);
    if (prog.statements == NULL)
        TEST_FAIL_MESSAGE("program.statements NULL");

    check_parser_errors(p);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

    Node n = prog.statements[0];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_ExpressionStatement, n.typ, "assert ExpressionStatement");

    ExpressionStatement* st = n.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            n_IntegerLiteral, st->expression.typ, "assert IntegerLiteral");

    IntegerLiteral* int_lit = st->expression.obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            5, int_lit->value, "assert IntegerLiteral.value");

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            "5", int_lit->tok.literal, "IntegerLiteral.Token.Literal");

    program_destroy(&prog);
    parser_destroy(p);
}

void test_parsing_prefix_expressions(void) {
    struct Test {
        char* input;
        char* operator;
        long integer_value;
    } tests[] = {
        {"!5;", "!", 5},
        {"-15;", "-", 15},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        if (prog.statements == NULL)
            TEST_FAIL_MESSAGE("program.statements NULL");

        check_parser_errors(p);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

        Node n = prog.statements[0];
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_ExpressionStatement, n.typ, "assert ExpressionStatement");

        ExpressionStatement* st = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_PrefixExpression, st->expression.typ, "assert PrefixExpression");

        PrefixExpression* pe = st->expression.obj;
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.operator, pe->op, "assert PrefixExpression.op");

        test_integer_literal(pe->right, test.integer_value);

        program_destroy(&prog);
        parser_destroy(p);
    }
}

void test_parsing_infix_expressions(void) {
    struct Test {
        char* input;
        long left_value;
        char* operator;
        long right_value;
    } tests[] = {
        {"5 + 5;", 5, "+", 5},
        {"5 - 5;", 5, "-", 5},
        {"5 * 5;", 5, "*", 5},
        {"5 / 5;", 5, "/", 5},
        {"5 > 5;", 5, ">", 5},
        {"5 < 5;", 5, "<", 5},
        {"5 == 5;", 5, "==", 5},
        {"5 != 5;", 5, "!=", 5},
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        if (prog.statements == NULL)
            TEST_FAIL_MESSAGE("program.statements NULL");

        check_parser_errors(p);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

        Node n = prog.statements[0];
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_ExpressionStatement, n.typ, "assert ExpressionStatement");

        ExpressionStatement* st = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_InfixExpression, st->expression.typ, "assert InfixExpression");

        InfixExpression* ie = st->expression.obj;

        test_integer_literal(ie->right, test.left_value);

        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                test.operator, ie->op, "assert InfixExpression.op");

        test_integer_literal(ie->right, test.right_value);

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
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        if (prog.statements == NULL)
            TEST_FAIL_MESSAGE("program.statements NULL");
        check_parser_errors(p);

        Node n = prog.statements[0];
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_ExpressionStatement, n.typ, "assert ExpressionStatement");

        size_t len = strlen(test.expected) + 2;
        char* buf = malloc(len * sizeof(char));
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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_let_statements);
    RUN_TEST(test_return_statements);
    RUN_TEST(test_identifier_expression);
    RUN_TEST(test_integer_literal_expression);
    RUN_TEST(test_parsing_prefix_expressions);
    RUN_TEST(test_parsing_infix_expressions);
    RUN_TEST(test_operator_precedence_parsing);
    return UNITY_END();
}
