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

void check_parser_errors(Parser* p) {
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
    TEST_ASSERT_EQUAL_STRING_MESSAGE(token_literal(&stmt), "let",
            "assert LetStatement.Token.Literal");
    TEST_ASSERT_EQUAL_INT_MESSAGE(n_LetStatement, stmt.typ,
            "assert type LetStatement");

    LetStatement* let_stmt = (LetStatement*)stmt.obj;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(exp_name, let_stmt->name->value,
            "assert LetStatement.Identifier.Name");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(exp_name, let_stmt->name->tok.literal,
            "assert LetStatement.Identifier.Token.Literal");
}

void test_let_statements(void) {
    char* input = "\
let x 5;\
let = 10;\
let 838383;\
";
    size_t input_len = strlen(input);

    Lexer l = lexer_new(input, input_len);
    Parser p = parser_new(&l);
    Program prog = parse_program(&p);
    if (prog.statements == NULL) {
        TEST_FAIL_MESSAGE("program.statements NULL");
    }
    check_parser_errors(&p);
    TEST_ASSERT_EQUAL_INT(3, prog.len);

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

    parser_destroy(&p);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_let_statements);
    return UNITY_END();
}
