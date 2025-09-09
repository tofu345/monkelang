#include "lexer.h"
#include "token.h"
#include "utils.h"

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

Lexer lexer_new(const char* input) {
    Lexer l;
    l.col = 0;
    l.line = 1;
    l.input = input;
    l.input_len = strlen(input);
    l.position = 0;
    l.read_position = 0;
    read_char(&l);
    return l;
}

inline static void
new_token(Token* t, TokenType type, char ch) {
    // must malloc here
    char* lit = malloc(2 * sizeof(char));
    if (lit == NULL) ALLOC_FAIL();
    lit[0] = ch;
    lit[1] = '\0';
    t->literal = lit;
    t->type = type;
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

static char* copy_string(const char* str, int from, int len) {
    char* ident = malloc((len + 1) * sizeof(char));
    if (ident == NULL) ALLOC_FAIL();
    for (int i = 0; i < len; i++) {
        ident[i] = str[from + i];
    }
    ident[len] = '\0';
    return ident;
}

static char* read_string(Lexer* l) {
    read_char(l);
    int position = l->position;
    int len = 0;
    while (!(l->ch == '"' || l->ch == 0)) {
        read_char(l);
        len++;
    }
    if (l->ch != '"') return NULL;
    return copy_string(l->input, position, len);
}

static char* read_identifier(Lexer* l) {
    int position = l->position;
    int len = 0;
    while (is_letter(l->ch)) {
        read_char(l);
        len++;
    }
    return copy_string(l->input, position, len);
}

static char* read_digit(Lexer* l, TokenType* t) {
    *t = t_Int;

    char peek = peek_char(l);
    bool (*check_fn) (char) = &is_digit;
    if (peek == 'x') check_fn = &is_hex_digit;
    else if (peek == 'b') check_fn = &is_binary_digit;

    bool is_float = false;
    int position = l->position;
    int len = 0;
    while (check_fn(l->ch)) {
        is_float = is_float || l->ch == '.';
        read_char(l);
        len++;
    }
    if (is_float) *t = t_Float;

    return copy_string(l->input, position, len);
}

Token lexer_next_token(Lexer* l) {
    skip_whitespace(l);

    Token tok = {};
    tok.line = l->line;
    tok.col = l->col;
    switch (l->ch) {
    case '=':
        if (peek_char(l) == '=') {
            char ch = l->ch;
            read_char(l);
            char* lit = malloc(3 * sizeof(char));
            if (lit == NULL) ALLOC_FAIL();
            lit[0] = ch;
            lit[1] = l->ch;
            lit[2] = '\0';
            tok = (Token){ t_Eq, l->col - 1, l->line, lit };
        } else {
            new_token(&tok, t_Assign, l->ch);
        }
        break;
    case '!':
        if (peek_char(l) == '=') {
            char ch = l->ch;
            read_char(l);
            char* lit = malloc(3 * sizeof(char));
            if (lit == NULL) ALLOC_FAIL();
            lit[0] = ch;
            lit[1] = l->ch;
            lit[2] = '\0';
            tok = (Token){ t_Not_eq, l->col - 1, l->line, lit };
        } else {
            new_token(&tok, t_Bang, l->ch);
        }
        break;
    case '+':
        new_token(&tok, t_Plus, l->ch);
        break;
    case '-':
        new_token(&tok, t_Minus, l->ch);
        break;
    case '/':
        new_token(&tok, t_Slash, l->ch);
        break;
    case '*':
        new_token(&tok, t_Asterisk, l->ch);
        break;
    case '<':
        new_token(&tok, t_Lt, l->ch);
        break;
    case '>':
        new_token(&tok, t_Gt, l->ch);
        break;
    case ';':
        new_token(&tok, t_Semicolon, l->ch);
        break;
    case ',':
        new_token(&tok, t_Comma, l->ch);
        break;
    case '(':
        new_token(&tok, t_Lparen, l->ch);
        break;
    case ')':
        new_token(&tok, t_Rparen, l->ch);
        break;
    case '{':
        new_token(&tok, t_Lbrace, l->ch);
        break;
    case '}':
        new_token(&tok, t_Rbrace, l->ch);
        break;
    case '[':
        new_token(&tok, t_Lbracket, l->ch);
        break;
    case ']':
        new_token(&tok, t_Rbracket, l->ch);
        break;
    case '"':
        tok.type = t_String;
        tok.literal = read_string(l);
        if (tok.literal == NULL)
            tok.type = t_Illegal;
        break;
    case 0:
        tok.literal = NULL;
        tok.type = t_Eof;
        break;
    default:
        if (is_letter(l->ch)) {
            tok.literal = read_identifier(l);
            tok.type = lookup_ident(tok.literal);
            return tok;
        } else if (is_digit(l->ch)) {
            tok.literal = read_digit(l, &tok.type);
            return tok;
        } else {
            new_token(&tok, t_Illegal, l->ch);
        }
    }

    read_char(l);
    return tok;
}
