#include "unity/unity.h"
#include "lexer.h"
#include "token.h"
#include "unity/unity_internals.h"

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
\"foobar\"\
\"foo bar\"\
[1, 2];\
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
        {t_String, "foobar"},
        {t_String, "foo bar"},
        {t_Lbracket, "["},
        {t_Int, "1"},
        {t_Comma, ","},
        {t_Int, "2"},
        {t_Rbracket, "]"},
        {t_Semicolon, ";"},
		{t_Eof, ""},
    };

    size_t len_tests = sizeof(tests) / sizeof(tests[0]);
    Lexer l = lexer_new(input);
    for (size_t i = 0; i < len_tests; i++) {
        char* msg = NULL;
        if (asprintf(&msg, "tests[%d]", (int)i) == -1) {
            TEST_FAIL_MESSAGE("no memory");
        };

        struct Test t = tests[i];
        Token tok = lexer_next_token(&l);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                show_token_type(t.exp_typ), show_token_type(tok.type), msg);
        if (t.exp_typ == t_Eof) {
            TEST_ASSERT_NULL(tok.literal);
        } else {
            TEST_ASSERT_EQUAL_STRING_MESSAGE(t.exp_lit, tok.literal, msg);
            free(tok.literal);
        }

        free(msg);
    }
}

void test_lexer_line_col(void) {
    char input[] = "let five = 5;\n\
let ten = 10;\n\
\n\
let add = fn(x, y) {\n\
    x + y;\n\
};\n\
\n\
10 == 10;\n\
";

    struct Test {
        TokenType exp_typ;
        char* exp_lit;
        char* line_col;
    } tests[] = {
		{t_Let, "let", "1,1"},
		{t_Ident, "five", "1,5"},
		{t_Assign, "=", "1,10"},
		{t_Int, "5", "1,12"},
		{t_Semicolon, ";", "1,13"},
		{t_Let, "let", "2,1"},
		{t_Ident, "ten", "2,5"},
		{t_Assign, "=", "2,9"},
		{t_Int, "10", "2,11"},
		{t_Semicolon, ";", "2,13"},
		{t_Let, "let", "4,1"},
		{t_Ident, "add", "4,5"},
		{t_Assign, "=", "4,9"},
		{t_Function, "fn", "4,11"},
		{t_Lparen, "(", "4,13"},
		{t_Ident, "x", "4,14"},
		{t_Comma, ",", "4,15"},
		{t_Ident, "y", "4,17"},
		{t_Rparen, ")", "4,18"},
		{t_Lbrace, "{", "4,20"},
		{t_Ident, "x", "5,5"},
		{t_Plus, "+", "5,7"},
		{t_Ident, "y", "5,9"},
		{t_Semicolon, ";", "5,10"},
		{t_Rbrace, "}", "6,1"},
		{t_Semicolon, ";", "6,2"},
		{t_Int, "10", "8,1"},
		{t_Eq, "==", "8,4"},
		{t_Int, "10", "8,7"},
		{t_Semicolon, ";", "8,9"},
		{t_Eof, "", "9,1"},
    };

    size_t len_tests = sizeof(tests) / sizeof(tests[0]);
    Lexer l = lexer_new(input);
    for (size_t i = 0; i < len_tests; i++) {
        char* msg = NULL;
        if (asprintf(&msg, "tests[%d]", (int)i) == -1) {
            TEST_FAIL_MESSAGE("no memory");
        };

        struct Test t = tests[i];
        Token tok = lexer_next_token(&l);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(
                show_token_type(t.exp_typ), show_token_type(tok.type), msg);
        if (t.exp_typ == t_Eof) {
            TEST_ASSERT_NULL(tok.literal);
        } else {
            TEST_ASSERT_EQUAL_STRING_MESSAGE(t.exp_lit, tok.literal, msg);
            free(tok.literal);
        }

        char* line_col = NULL;
        if (asprintf(&line_col, "%d,%d", tok.line, tok.col) == -1) {
            TEST_FAIL_MESSAGE("no memory");
        };

        TEST_ASSERT_EQUAL_STRING_MESSAGE(t.line_col, line_col, msg);

        free(line_col);
        free(msg);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lexer_next_token);
    RUN_TEST(test_lexer_line_col);
    return UNITY_END();
}
