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
		{Let, "let"},
		{Ident, "five"},
		{Assign, "="},
		{Int, "5"},
		{Semicolon, ";"},
		{Let, "let"},
		{Ident, "ten"},
		{Assign, "="},
		{Int, "10"},
		{Semicolon, ";"},
		{Let, "let"},
		{Ident, "add"},
		{Assign, "="},
		{Function, "fn"},
		{Lparen, "("},
		{Ident, "x"},
		{Comma, ","},
		{Ident, "y"},
		{Rparen, ")"},
		{Lbrace, "{"},
		{Ident, "x"},
		{Plus, "+"},
		{Ident, "y"},
		{Semicolon, ";"},
		{Rbrace, "}"},
		{Semicolon, ";"},
		{Let, "let"},
		{Ident, "result"},
		{Assign, "="},
		{Ident, "add"},
		{Lparen, "("},
		{Ident, "five"},
		{Comma, ","},
		{Ident, "ten"},
		{Rparen, ")"},
		{Semicolon, ";"},
		{Bang, "!"},
		{Minus, "-"},
		{Slash, "/"},
		{Asterisk, "*"},
		{Int, "5"},
		{Semicolon, ";"},
		{Int, "5"},
		{Lt, "<"},
		{Int, "10"},
		{Gt, ">"},
		{Int, "5"},
		{Semicolon, ";"},
		{If, "if"},
		{Lparen, "("},
		{Int, "5"},
		{Lt, "<"},
		{Int, "10"},
		{Rparen, ")"},
		{Lbrace, "{"},
		{Return, "return"},
		{True, "true"},
		{Semicolon, ";"},
		{Rbrace, "}"},
		{Else, "else"},
		{Lbrace, "{"},
		{Return, "return"},
		{False, "false"},
		{Semicolon, ";"},
		{Rbrace, "}"},
		{Int, "10"},
		{Eq, "=="},
		{Int, "10"},
		{Semicolon, ";"},
		{Int, "10"},
		{Not_eq, "!="},
		{Int, "9"},
		{Semicolon, ";"},
		{Eof, ""},
    };

    size_t len = sizeof(input) / sizeof(input[0]);
    size_t len_tests = sizeof(tests) / sizeof(tests[0]);
    char msg[10];
    Lexer* l = lexer_new(input, len);
    for (size_t i = 0; i < len_tests; i++) {
        struct Test t = tests[i];
        Token tok = lexer_next_token(l);
        sprintf(msg, "tests[%d]", (int)i);

        TEST_ASSERT_EQUAL_INT_MESSAGE(t.exp_typ, tok.type, msg);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(t.exp_lit, tok.literal, msg);

        free(tok.literal);
    }
    lexer_destroy(l);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lexer_next_token);
    return UNITY_END();
}
