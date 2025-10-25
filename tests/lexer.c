#include "unity/unity.h"

#include "../src/lexer.h"
#include "../src/token.h"

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
\"foo\nbar\"\
[1, 2];\
{\"foo\": \"bar\"};\
// this should be ignored";

    struct Test {
        TokenType type;
        char* str;
    } tests[] = {
	{t_Let, "let"},
	{t_Ident, "five"},
	{t_Assign, "="},
	{t_Digit, "5"},
	{t_Semicolon, ";"},
	{t_Let, "let"},
	{t_Ident, "ten"},
	{t_Assign, "="},
	{t_Digit, "10"},
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
	{t_Digit, "5"},
	{t_Semicolon, ";"},
	{t_Digit, "5"},
	{t_Lt, "<"},
	{t_Digit, "10"},
	{t_Gt, ">"},
	{t_Digit, "5"},
	{t_Semicolon, ";"},
	{t_If, "if"},
	{t_Lparen, "("},
	{t_Digit, "5"},
	{t_Lt, "<"},
	{t_Digit, "10"},
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
	{t_Digit, "10"},
	{t_Eq, "=="},
	{t_Digit, "10"},
	{t_Semicolon, ";"},
	{t_Digit, "10"},
	{t_Not_eq, "!="},
	{t_Digit, "9"},
	{t_Semicolon, ";"},
	{t_String, "foobar"},
	{t_String, "foo bar"},
	{t_String, "foo\nbar"},
	{t_Lbracket, "["},
	{t_Digit, "1"},
	{t_Comma, ","},
	{t_Digit, "2"},
	{t_Rbracket, "]"},
	{t_Semicolon, ";"},
	{t_Lbrace, "{"},
	{t_String, "foo"},
	{t_Colon, ":"},
	{t_String, "bar"},
	{t_Rbrace, "}"},
	{t_Semicolon, ";"},
	{t_Eof, ""},
    };

    size_t len_tests = sizeof(tests) / sizeof(tests[0]);
    Lexer l;
    lexer_init(&l, input);
    for (size_t i = 0; i < len_tests; i++) {
        struct Test t = tests[i];
        Token tok =
            lexer_next_token(&l);

        if (t.type != tok.type) {
            printf("test[%zu]: invalid token type, expected '%s' got '%s'\n",
                    i, show_token_type(t.type), show_token_type(tok.type));
            TEST_FAIL();
        }

        if (t.type == t_Eof) {
            TEST_ASSERT_NULL(tok.start);

        } else if (strncmp(t.str, tok.start, tok.length) != 0) {
            printf("test[%zu]: invalid token literal, expected '%s' got '%.*s'\n",
                    i, t.str, tok.length, tok.start);
            TEST_FAIL();
        }
    }
}

void test_lexer_line(void) {
    char input[] = "let five = 5;\n\
let ten = 10;\n\
\n\
let add = fn(x, y) {\n\
    x + y;\n\
};\n\
\n\
// ignore\n\
10 == 10;\n\
";

    struct Test {
        TokenType type;
        char* str;
        int line;
    } tests[] = {
	{t_Let, "let", 1},
	{t_Ident, "five", 1},
	{t_Assign, "=", 1},
	{t_Digit, "5", 1},
	{t_Semicolon, ";", 1},
	{t_Let, "let", 2},
	{t_Ident, "ten", 2},
	{t_Assign, "=", 2},
	{t_Digit, "10", 2},
	{t_Semicolon, ";", 2},
	{t_Let, "let", 4},
	{t_Ident, "add", 4},
	{t_Assign, "=", 4},
	{t_Function, "fn", 4},
	{t_Lparen, "(", 4},
	{t_Ident, "x", 4},
	{t_Comma, ",", 4},
	{t_Ident, "y", 4},
	{t_Rparen, ")", 4},
	{t_Lbrace, "{", 4},
	{t_Ident, "x", 5},
	{t_Plus, "+", 5},
	{t_Ident, "y", 5},
	{t_Semicolon, ";", 5},
	{t_Rbrace, "}", 6},
	{t_Semicolon, ";", 6},
	{t_Digit, "10", 9},
	{t_Eq, "==", 9},
	{t_Digit, "10", 9},
	{t_Semicolon, ";", 9},
	{t_Eof, "", 10},
    };

    size_t len_tests = sizeof(tests) / sizeof(tests[0]);
    Lexer l;
    lexer_init(&l, input);
    for (size_t i = 0; i < len_tests; i++) {
        struct Test t = tests[i];
        Token tok = lexer_next_token(&l);

        if (t.type != tok.type) {
            printf("test[%zu]: invalid token type, expected '%s' got '%s'\n",
                    i, show_token_type(t.type), show_token_type(tok.type));
            TEST_FAIL();
        }

        if (t.type == t_Eof) {
            TEST_ASSERT_NULL(tok.start);

        } else if (strncmp(t.str, tok.start, tok.length) != 0) {
            printf("test[%zu]: invalid token literal, expected '%s' got '%.*s'\n",
                    i, t.str, tok.length, tok.start);
            TEST_FAIL();
        }

        if (t.line != tok.line) {
            printf("test[%zu]: invalid token line, expected %d got %d\n",
                    i, t.line, tok.line);
            TEST_FAIL();
        }
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lexer_next_token);
    RUN_TEST(test_lexer_line);
    return UNITY_END();
}
