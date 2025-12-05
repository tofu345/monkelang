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
let digitAnd20 = 50;\
let nullVariable = null;\
let e = 2;\
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
// this should be ignored\n\
\
a += 1;\
a -= 1;\
a /= 1;\
a *= 1;\
\
for (let i = 0; i < 10; i = i + 1) {\
    puts(\"i:\", i);\
}";

    struct Test {
        TokenType type;
        char* str;
        int length;
    } tests[] = {
	{t_Let, "let", 3},
	{t_Ident, "five", 4},
	{t_Assign, "=", 1},
	{t_Integer, "5", 1},
	{t_Semicolon, ";", 1},
	{t_Let, "let", 3},
	{t_Ident, "ten", 3},
	{t_Assign, "=", 1},
	{t_Integer, "10", 2},
	{t_Semicolon, ";", 1},
	{t_Let, "let", 3},
	{t_Ident, "digitAnd20", 10},
	{t_Assign, "=", 1},
	{t_Integer, "50", 2},
	{t_Semicolon, ";", 1},
	{t_Let, "let", 3},
	{t_Ident, "nullVariable", 12},
	{t_Assign, "=", 1},
	{t_Null, "null", 4},
	{t_Semicolon, ";", 1},
	{t_Let, "let", 3},
	{t_Ident, "e", 1},
	{t_Assign, "=", 1},
	{t_Integer, "2", 1},
	{t_Semicolon, ";", 1},
	{t_Let, "let", 3},
	{t_Ident, "add", 3},
	{t_Assign, "=", 1},
	{t_Function, "fn", 2},
	{t_Lparen, "(", 1},
	{t_Ident, "x", 1},
	{t_Comma, ",", 1},
	{t_Ident, "y", 1},
	{t_Rparen, ")", 1},
	{t_Lbrace, "{", 1},
	{t_Ident, "x", 1},
	{t_Plus, "+", 1},
	{t_Ident, "y", 1},
	{t_Semicolon, ";", 1},
	{t_Rbrace, "}", 1},
	{t_Semicolon, ";", 1},
	{t_Let, "let", 3},
	{t_Ident, "result", 6},
	{t_Assign, "=", 1},
	{t_Ident, "add", 3},
	{t_Lparen, "(", 1},
	{t_Ident, "five", 4},
	{t_Comma, ",", 1},
	{t_Ident, "ten", 3},
	{t_Rparen, ")", 1},
	{t_Semicolon, ";", 1},
	{t_Bang, "!", 1},
	{t_Minus, "-", 1},
	{t_Slash, "/", 1},
	{t_Asterisk, "*", 1},
	{t_Integer, "5", 1},
	{t_Semicolon, ";", 1},
	{t_Integer, "5", 1},
	{t_Lt, "<", 1},
	{t_Integer, "10", 2},
	{t_Gt, ">", 1},
	{t_Integer, "5", 1},
	{t_Semicolon, ";", 1},
	{t_If, "if", 2},
	{t_Lparen, "(", 1},
	{t_Integer, "5", 1},
	{t_Lt, "<", 1},
	{t_Integer, "10", 2},
	{t_Rparen, ")", 1},
	{t_Lbrace, "{", 1},
	{t_Return, "return", 6},
	{t_True, "true", 4},
	{t_Semicolon, ";", 1},
	{t_Rbrace, "}", 1},
	{t_Else, "else", 4},
	{t_Lbrace, "{", 1},
	{t_Return, "return", 6},
	{t_False, "false", 5},
	{t_Semicolon, ";", 1},
	{t_Rbrace, "}", 1},
	{t_Integer, "10", 2},
	{t_Eq, "==", 2},
	{t_Integer, "10", 2},
	{t_Semicolon, ";", 1},
	{t_Integer, "10", 2},
	{t_Not_eq, "!=", 2},
	{t_Integer, "9", 1},
	{t_Semicolon, ";", 1},
	{t_String, "foobar", 6},
	{t_String, "foo bar", 7},
	{t_String, "foo\nbar", 7},
	{t_Lbracket, "[", 1},
	{t_Integer, "1", 1},
	{t_Comma, ",", 1},
	{t_Integer, "2", 1},
	{t_Rbracket, "]", 1},
	{t_Semicolon, ";", 1},
	{t_Lbrace, "{", 1},
	{t_String, "foo", 3},
	{t_Colon, ":", 1},
	{t_String, "bar", 3},
	{t_Rbrace, "}", 1},
	{t_Semicolon, ";", 1},
	{t_Ident, "a", 1},
	{t_Add_Assign, "+=", 2},
	{t_Integer, "1", 1},
	{t_Semicolon, ";", 1},
	{t_Ident, "a", 1},
	{t_Sub_Assign, "-=", 2},
	{t_Integer, "1", 1},
	{t_Semicolon, ";", 1},
	{t_Ident, "a", 1},
	{t_Div_Assign, "/=", 2},
	{t_Integer, "1", 1},
	{t_Semicolon, ";", 1},
	{t_Ident, "a", 1},
	{t_Mul_Assign, "*=", 2},
	{t_Integer, "1", 1},
	{t_Semicolon, ";", 1},
	{t_For, "for", 3},
	{t_Lparen, "(", 1},
	{t_Let, "let", 3},
	{t_Ident, "i", 1},
	{t_Assign, "=", 1},
	{t_Integer, "0", 1},
	{t_Semicolon, ";", 1},
	{t_Ident, "i", 1},
	{t_Lt, "<", 1},
	{t_Integer, "10", 2},
	{t_Semicolon, ";", 1},
	{t_Ident, "i", 1},
	{t_Assign, "=", 1},
	{t_Ident, "i", 1},
	{t_Plus, "+", 1},
	{t_Integer, "1", 1},
	{t_Rparen, ")", 1},
	{t_Lbrace, "{", 1},
	{t_Ident, "puts", 4},
	{t_Lparen, "(", 1},
	{t_String, "i:", 2},
	{t_Comma, ",", 1},
	{t_Ident, "i", 1},
	{t_Rparen, ")", 1},
	{t_Semicolon, ";", 1},
	{t_Rbrace, "}", 1},
	{t_Eof, "", 1},
    };
size_t len_tests = sizeof(tests) / sizeof(tests[0]);

    Lexer l;
    lexer_init(&l, input);

    for (size_t i = 0; i < len_tests; i++) {
        struct Test test = tests[i];
        Token tok =
            lexer_next_token(&l);

        if (test.type != tok.type) {
            printf("test[%zu] %.*s: invalid token type, expected '%s' got '%s'\n",
                    i, LITERAL(tok),
                    show_token_type(test.type), show_token_type(tok.type));
            TEST_FAIL();
        }

        if (strncmp(test.str, tok.start, tok.length) != 0) {
            printf("test[%zu]: invalid token literal, expected '%s' got '%.*s'\n",
                    i, test.str, tok.length, tok.start);
            TEST_FAIL();
        }

        if (test.length != tok.length) {
            printf("test[%zu] %.*s: invalid token length, expected %d got %d\n",
                    i, LITERAL(tok), test.length, tok.length);
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
	{t_Integer, "5", 1},
	{t_Semicolon, ";", 1},
	{t_Let, "let", 2},
	{t_Ident, "ten", 2},
	{t_Assign, "=", 2},
	{t_Integer, "10", 2},
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
	{t_Integer, "10", 9},
	{t_Eq, "==", 9},
	{t_Integer, "10", 9},
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

        if (strncmp(t.str, tok.start, tok.length) != 0) {
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
