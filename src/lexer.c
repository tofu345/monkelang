#include "lexer.h"
#include "token.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef bool check_function(char ch);

inline static void
read_char(Lexer *l) {
    l->position = l->read_position;

    if (l->position >= l->input_len) {
        l->ch = 0;
        return;

    } else {
        l->ch = l->input[l->position];
    }

    l->read_position += 1;

    if (l->ch == '\n') {
        l->line++;
    }
}

void lexer_init(Lexer *l, const char *input, uint64_t length) {
    *l = (Lexer) {
        .input = NULL,
        .input_len = 0,
        .ch = 0,
        .position = 0,
        .read_position = 0,
        .line = 1,
    };

    if (input) {
        l->input = input;
        l->input_len = length;
        read_char(l);
    }
}

void lexer_with(Lexer *l, const char *input, uint64_t length) {
    l->input = input;
    l->input_len = length;
    l->position = 0;
    l->read_position = 0;
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
is_identifier(char ch) {
    return is_letter(ch) || is_digit(ch);
}

inline static bool
is_whitespace(char ch) {
    return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
}

inline static bool
not_closing_quote(char ch) {
    return ch != '"';
}

// read while [check(l->ch)], return number of read characters.
static int
read_while(Lexer *l, check_function *check) {
    int pos = l->position;

    while (l->position < l->input_len && check(l->ch)) {
        read_char(l);
    }

    return l->position - pos;
}

Token lexer_next_token(Lexer *l) {
    read_while(l, is_whitespace);

    Token tok = {
        .type = t_Illegal,
        .line = l->line,
        .start = l->input + l->position,
        .length = 1,
        .position = l->position,
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
        if (peek_char(l) == '=') {
            read_char(l);
            tok.type = t_Add_Assign;
            tok.length = 2;
        } else {
            tok.type = t_Plus;
        }
        break;

    case '-':
        if (peek_char(l) == '=') {
            read_char(l);
            tok.type = t_Sub_Assign;
            tok.length = 2;
        } else {
            tok.type = t_Minus;
        }
        break;

    case '/':
        if (peek_char(l) == '/') {
            // comments
            uint64_t pos = l->read_position + 1;
            while (pos < l->input_len && l->input[pos] != '\n') {
                pos++;
            }
            l->read_position = pos;
            read_char(l);
            return lexer_next_token(l);

        } else if (peek_char(l) == '=') {
            read_char(l);
            tok.type = t_Div_Assign;
            tok.length = 2;

        } else {
            tok.type = t_Slash;
        }
        break;

    case '*':
        if (peek_char(l) == '=') {
            read_char(l);
            tok.type = t_Mul_Assign;
            tok.length = 2;
        } else {
            tok.type = t_Asterisk;
        }
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
        ++tok.position;
        tok.length = read_while(l, not_closing_quote);
        break;

    case 0:
        tok.type = t_Eof;
        return tok;

    default:
        if (is_letter(l->ch)) {
            tok.length = read_while(l, is_identifier);
            tok.type = lookup_ident(tok.start, tok.length);
            return tok;

        } else if (is_digit(l->ch)) {
            tok.type = t_Integer;

            int pos = l->position;
            while (l->position < l->input_len && is_digit(l->ch)) {
                if (l->ch == '.') {
                    tok.type = t_Float;
                }
                read_char(l);
            }
            tok.length = l->position - pos;
            return tok;
        }
    }

    read_char(l);
    return tok;
}
