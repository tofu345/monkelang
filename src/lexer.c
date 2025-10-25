#include "lexer.h"
#include "token.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef bool check_function(char ch);

inline static void
read_char(Lexer *l) {
    if (l->read_position >= l->input_len) {
        l->ch = 0;
    } else {
        l->ch = l->input[l->read_position];
    }

    l->position = l->read_position;
    l->read_position += 1;

    if (l->ch == '\n') {
        l->line++;
    }
}

void lexer_init(Lexer *l, const char *input) {
    *l = (Lexer) {
        .input = input,
        .input_len = strlen(input),
        .ch = 0,
        .line = 1,
        .position = 0,
        .read_position = 0,
    };
    read_char(l);
}

static char peek_char(Lexer *l) {
    if (l->read_position >= l->input_len) {
        return 0;
    } else {
        return l->input[l->read_position];
    }
}

inline static bool
is_letter(char ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

inline static bool
is_digit(char ch) {
    return ('0' <= ch && ch <= '9') || ch == '.';
}

inline static bool
is_whitespace(char ch) {
    switch (ch) {
        case ' ':
        case '\n':
        case '\t':
        case '\r':
            return true;

        default:
            return false;
    }
}

inline static bool
not_closing_quote(char ch) {
    return ch != '"';
}

// read while [check(l->ch)], return number of read characters.
static int
read_while(Lexer *l, check_function *check) {
    int pos = l->read_position;

    while (l->ch != 0 && check(l->ch)) {
        read_char(l);
    }

    return l->read_position - pos;
}

static void
skip_whitespace(Lexer *l) {
    read_while(l, is_whitespace);
}

Token lexer_next_token(Lexer *l) {
    skip_whitespace(l);

    Token tok = {
        .type = t_Illegal,
        .line = l->line,
        .start = l->input + l->position,
        .length = 1,
    };

    switch (l->ch) {
    case '=':
        if (peek_char(l) == '=') {
            read_char(l);
            tok.type = t_Eq;
            tok.length = 2;

        } else {
            tok.type = t_Assign;
        }
        break;

    case '!':
        if (peek_char(l) == '=') {
            read_char(l);
            tok.type = t_Not_eq;
            tok.length = 2;

        } else {
            tok.type = t_Bang;
        }
        break;

    case '+':
        tok.type = t_Plus;
        break;

    case '-':
        tok.type = t_Minus;
        break;

    case '/':
        if (peek_char(l) == '/') {
            while (l->ch != 0 && l->ch != '\n') {
                read_char(l);
            }
            return lexer_next_token(l);

        } else {
            tok.type = t_Slash;
        }
        break;

    case '*':
        tok.type = t_Asterisk;
        break;

    case '<':
        tok.type = t_Lt;
        break;

    case '>':
        tok.type = t_Gt;
        break;

    case ';':
        tok.type = t_Semicolon;
        break;

    case ':':
        tok.type = t_Colon;
        break;

    case ',':
        tok.type = t_Comma;
        break;

    case '(':
        tok.type = t_Lparen;
        break;

    case ')':
        tok.type = t_Rparen;
        break;

    case '{':
        tok.type = t_Lbrace;
        break;

    case '}':
        tok.type = t_Rbrace;
        break;

    case '[':
        tok.type = t_Lbracket;
        break;

    case ']':
        tok.type = t_Rbracket;
        break;

    case '"':
        read_char(l);
        tok.type = t_String;
        // exclude quotes
        ++tok.start;
        tok.length = read_while(l, not_closing_quote);
        break;

    case 0:
        tok.type = t_Eof;
        tok.start = NULL;
        return tok;

    default:
        if (is_letter(l->ch)) {
            const char *start = l->input + l->position;
            tok.length = read_while(l, is_letter);
            tok.type = lookup_ident(start, tok.length);
            return tok;

        } else if (is_digit(l->ch)) {
            tok.type = t_Digit;
            tok.length = read_while(l, is_digit);
            return tok;
        }
    }

    read_char(l);
    return tok;
}
