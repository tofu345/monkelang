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
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.statements, "program.statements NULL");

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
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.statements, "program.statements NULL");

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
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.statements, "program.statements NULL");

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
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.statements, "program.statements NULL");

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
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.statements, "program.statements NULL");

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
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.statements, "program.statements NULL");

        check_parser_errors(p);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

        Node n = prog.statements[0];
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_ExpressionStatement, n.typ, "assert ExpressionStatement");

        ExpressionStatement* st = n.obj;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
                n_InfixExpression, st->expression.typ, "assert InfixExpression");

        InfixExpression* ie = st->expression.obj;

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
    };
    size_t tests_len = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_len; i++) {
        struct Test test = tests[i];

        Lexer l = lexer_new(test.input, strlen(test.input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.statements, "program.statements NULL");
        check_parser_errors(p);

        Node n = prog.statements[0];
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
        TEST_ASSERT_NOT_NULL_MESSAGE(prog.statements, "program.statements NULL");
        check_parser_errors(p);

        TEST_ASSERT_EQUAL_INT_MESSAGE(1, prog.len, "prog.statements length");

        Node n = prog.statements[0];
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
    return UNITY_END();
}
