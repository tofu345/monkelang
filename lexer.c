#include "lexer.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static void read_char(Lexer* l) {
    if (l->read_position >= l->input_len) {
        l->ch = 0;
    } else {
        l->ch = l->input[l->read_position];
    }

    l->position = l->read_position;
    l->read_position += 1;

    if (l->ch == '\n') {
        l->line++;
        l->col = 0;
    } else {
        l->col++;
    }
}

void lexer_init(Lexer* l, const char* input) {
    l->col = 0;
    l->line = 1;
    l->input = input;
    l->input_len = strlen(input);
    l->position = 0;
    l->read_position = 0;
    read_char(l);
}

Lexer lexer_new(const char* input) {
    Lexer l;
    lexer_init(&l, input);
    return l;
}

static Token new_token(Lexer* l, TokenType type, char ch) {
    // must malloc here
    char* lit = malloc(2 * sizeof(char));
    lit[0] = ch;
    lit[1] = '\0';
    return (Token){ type, l->col, l->line, lit };
}

static bool is_letter(char ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static bool is_digit(char ch) {
    return ('0' <= ch && ch <= '9') || ch == '.';
}

static bool is_hex_digit(char ch) {
    return ('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f')
        || ('A' <= ch && ch <= 'F') || ch == 'x';
}

static bool is_binary_digit(char ch) {
    return ch == '0' || ch == '1' || ch == 'b';
}

static void skip_whitespace(Lexer* l) {
    while (l->ch == ' ' || l->ch == '\t' || l->ch == '\n' || l->ch == '\r') {
        read_char(l);
    }
}

static char peek_char(Lexer *l) {
    if (l->read_position >= l->input_len) {
        return 0;
    } else {
        return l->input[l->read_position];
    }
}

static char* read_identifier(Lexer* l) {
    size_t position = l->position;
    int len = 0;
    while (is_letter(l->ch)) {
        read_char(l);
        len++;
    }
    char* ident = malloc((len + 1) * sizeof(char));
    int i = 0;
    for (; i < len; i++) {
        ident[i] = l->input[position + i];
    }
    ident[len] = '\0';
    return ident;
}

static char* read_digit(Lexer* l, TokenType* t) {
    *t = t_Int;

    bool (*check_fn) (char) = &is_digit;
    char peek = peek_char(l);
    if (peek == 'x') {
        check_fn = &is_hex_digit;
    } else if (peek == 'b') {
        check_fn = &is_binary_digit;
    }

    bool is_float = false;
    size_t position = l->position;
    int len = 0;
    while (check_fn(l->ch)) {
        is_float = is_float || l->ch == '.';
        read_char(l);
        len++;
    }
    if (is_float)
        *t = t_Float;

    char* ident = malloc((len + 1) * sizeof(char));
    int i = 0;
    for (; i < len; i++) {
        ident[i] = l->input[position + i];
    }
    ident[len] = '\0';
    return ident;
}

Token lexer_next_token(Lexer* l) {
    Token tok = {};
    skip_whitespace(l);

    switch (l->ch) {
    case '=':
        if (peek_char(l) == '=') {
            char ch = l->ch;
            read_char(l);
            char* lit = malloc(3 * sizeof(char));
            lit[0] = ch;
            lit[1] = l->ch;
            lit[2] = '\0';
            tok = (Token){ t_Eq, l->col - 1, l->line, lit };
        } else {
            tok = new_token(l, t_Assign, l->ch);
        }
        break;
    case '!':
        if (peek_char(l) == '=') {
            char ch = l->ch;
            read_char(l);
            char* lit = malloc(3 * sizeof(char));
            lit[0] = ch;
            lit[1] = l->ch;
            lit[2] = '\0';
            tok = (Token){ t_Not_eq, l->col - 1, l->line, lit };
        } else {
            tok = new_token(l, t_Bang, l->ch);
        }
        break;
    case '+':
        tok = new_token(l, t_Plus, l->ch);
        break;
    case '-':
        tok = new_token(l, t_Minus, l->ch);
        break;
    case '/':
        tok = new_token(l, t_Slash, l->ch);
        break;
    case '*':
        tok = new_token(l, t_Asterisk, l->ch);
        break;
    case '<':
        tok = new_token(l, t_Lt, l->ch);
        break;
    case '>':
        tok = new_token(l, t_Gt, l->ch);
        break;
    case ';':
        tok = new_token(l, t_Semicolon, l->ch);
        break;
    case ',':
        tok = new_token(l, t_Comma, l->ch);
        break;
    case '(':
        tok = new_token(l, t_Lparen, l->ch);
        break;
    case ')':
        tok = new_token(l, t_Rparen, l->ch);
        break;
    case '{':
        tok = new_token(l, t_Lbrace, l->ch);
        break;
    case '}':
        tok = new_token(l, t_Rbrace, l->ch);
        break;
    case 0:
        tok.line = l->line;
        tok.col = l->col;
        tok.literal = NULL;
        tok.type = t_Eof;
        break;
    default:
        if (is_letter(l->ch)) {
            tok.line = l->line;
            tok.col = l->col;
            tok.literal = read_identifier(l);
            tok.type = lookup_ident(tok.literal);
            return tok;
        } else if (is_digit(l->ch)) {
            tok.line = l->line;
            tok.col = l->col;
            tok.literal = read_digit(l, &tok.type);
            return tok;
        } else {
            tok = new_token(l, t_Illegal, l->ch);
        }
    }

    read_char(l);
    return tok;
}
