#include "lexer.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static void read_char(Lexer* l) {
    if (l->read_position >= l->input_len) {
        l->ch = 0;
    } else {
        l->ch = l->input[l->read_position];
    }
    l->position = l->read_position;
    l->read_position += 1;
}

Lexer lexer_new(const char* input, size_t len) {
    Lexer l = {};
    l.input = input;
    l.input_len = len;
    read_char(&l);
    return l;
}

static Token new_token(TokenType type, char ch) {
    // must malloc here
    char* lit = malloc(2 * sizeof(char));
    lit[0] = ch;
    lit[1] = '\0';
    return (Token){ type, lit };
}

static bool is_letter(char ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static bool is_digit(char ch) {
    return '0' <= ch && ch <= '9';
}

static void skip_whitespace(Lexer* l) {
    while (l->ch == ' ' || l->ch == '\t' || l->ch == '\n' || l->ch == '\r') {
        read_char(l);
    }
}

static char* read_while(Lexer* l, bool (*is_ok) (char)) {
    size_t position = l->position;
    int len = 0;
    while (is_ok(l->ch)) {
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

static char peek_char(Lexer *l) {
    if (l->read_position >= l->input_len) {
        return 0;
    } else {
        return l->input[l->read_position];
    }
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
            tok = (Token){ t_Eq, lit };
        } else {
            tok = new_token(t_Assign, l->ch);
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
            tok = (Token){ t_Not_eq, lit };
        } else {
            tok = new_token(t_Bang, l->ch);
        }
        break;
    case '+':
        tok = new_token(t_Plus, l->ch);
        break;
    case '-':
        tok = new_token(t_Minus, l->ch);
        break;
    case '/':
        tok = new_token(t_Slash, l->ch);
        break;
    case '*':
        tok = new_token(t_Asterisk, l->ch);
        break;
    case '<':
        tok = new_token(t_Lt, l->ch);
        break;
    case '>':
        tok = new_token(t_Gt, l->ch);
        break;
    case ';':
        tok = new_token(t_Semicolon, l->ch);
        break;
    case ',':
        tok = new_token(t_Comma, l->ch);
        break;
    case '(':
        tok = new_token(t_Lparen, l->ch);
        break;
    case ')':
        tok = new_token(t_Rparen, l->ch);
        break;
    case '{':
        tok = new_token(t_Lbrace, l->ch);
        break;
    case '}':
        tok = new_token(t_Rbrace, l->ch);
        break;
    case 0:
        tok.literal = NULL;
        tok.type = t_Eof;
        break;
    default:
        if (is_letter(l->ch)) {
            tok.literal = read_while(l, is_letter);
            tok.type = lookup_ident(tok.literal);
            return tok;
        } else if (is_digit(l->ch)) {
            tok.type = t_Int;
            tok.literal = read_while(l, is_digit);
            return tok;
        } else {
            tok = new_token(t_Illegal, l->ch);
        }
    }

    read_char(l);
    return tok;
}
