#include "unity/unity.h"
#include "lexer.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

void test_lexer_next_token(void) {
    // who needs new lines?
    char input[] = "let five = 5;\
let ten = 10;\
\
let add = fn(x, y) {\
    x + y;\
};\
\
let result = add(five, ten);\
!-/*5;\
5 < 10 > 5;\
\
if (5 < 10) {\
	return true;\
} else {\
	return false;\
}\
\
10 == 10;\
10 != 9;\
";

    struct Test {
        TokenType exp_typ;
        char* exp_lit;
    } tests[] = {
		{t_Let, "let"},
		{t_Ident, "five"},
		{t_Assign, "="},
		{t_Int, "5"},
		{t_Semicolon, ";"},
		{t_Let, "let"},
		{t_Ident, "ten"},
		{t_Assign, "="},
		{t_Int, "10"},
		{t_Semicolon, ";"},
		{t_Let, "let"},
		{t_Ident, "add"},
		{t_Assign, "="},
		{t_Function, "fn"},
		{t_Lparen, "("},
		{t_Ident, "x"},
		{t_Comma, ","},
		{t_Ident, "y"},
		{t_Rparen, ")"},
		{t_Lbrace, "{"},
		{t_Ident, "x"},
		{t_Plus, "+"},
		{t_Ident, "y"},
		{t_Semicolon, ";"},
		{t_Rbrace, "}"},
		{t_Semicolon, ";"},
		{t_Let, "let"},
		{t_Ident, "result"},
		{t_Assign, "="},
		{t_Ident, "add"},
		{t_Lparen, "("},
		{t_Ident, "five"},
		{t_Comma, ","},
		{t_Ident, "ten"},
		{t_Rparen, ")"},
		{t_Semicolon, ";"},
		{t_Bang, "!"},
		{t_Minus, "-"},
		{t_Slash, "/"},
		{t_Asterisk, "*"},
		{t_Int, "5"},
		{t_Semicolon, ";"},
		{t_Int, "5"},
		{t_Lt, "<"},
		{t_Int, "10"},
		{t_Gt, ">"},
		{t_Int, "5"},
		{t_Semicolon, ";"},
		{t_If, "if"},
		{t_Lparen, "("},
		{t_Int, "5"},
		{t_Lt, "<"},
		{t_Int, "10"},
		{t_Rparen, ")"},
		{t_Lbrace, "{"},
		{t_Return, "return"},
		{t_True, "true"},
		{t_Semicolon, ";"},
		{t_Rbrace, "}"},
		{t_Else, "else"},
		{t_Lbrace, "{"},
		{t_Return, "return"},
		{t_False, "false"},
		{t_Semicolon, ";"},
		{t_Rbrace, "}"},
		{t_Int, "10"},
		{t_Eq, "=="},
		{t_Int, "10"},
		{t_Semicolon, ";"},
		{t_Int, "10"},
		{t_Not_eq, "!="},
		{t_Int, "9"},
		{t_Semicolon, ";"},
		{t_Eof, ""},
    };

    size_t len = sizeof(input) / sizeof(input[0]);
    size_t len_tests = sizeof(tests) / sizeof(tests[0]);
    Lexer l = lexer_new(input, len);
    for (size_t i = 0; i < len_tests; i++) {
        char* msg = NULL;
        if (asprintf(&msg, "tests[%d]", (int)i) == -1) {
            TEST_FAIL_MESSAGE("malloc msg");
        };

        struct Test t = tests[i];
        Token tok = lexer_next_token(&l);
        TEST_ASSERT_EQUAL_INT_MESSAGE(t.exp_typ, tok.type, msg);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(t.exp_lit, tok.literal, msg);

        free(msg);
        free(tok.literal);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lexer_next_token);
    return UNITY_END();
}
