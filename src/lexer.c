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

void lexer_init(Lexer *l, const char *input) {
    l->col = 0;
    l->line = 1;
    l->input = input;
    l->input_len = strlen(input);
    l->position = 0;
    l->read_position = 0;
    read_char(l);
}

static void
new_token(Token* t, TokenType type, char ch) {
    // must malloc here
    char* lit = malloc(2 * sizeof(char));
    if (lit == NULL) die("malloc");
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
        || ('A' <= ch && ch <= 'F');
}

static bool is_binary_digit(char ch) {
    return ch == '0' || ch == '1';
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

static char* copy_string(Lexer* l, int from, int len) {
    char* ident = malloc((len + 1) * sizeof(char));
    if (ident == NULL) die("malloc");
    memcpy(ident, l->input + from, len);
    ident[len] = '\0';
    return ident;
}

// read_string till closing quote, if not present return NULL.
static char* read_string(Lexer* l) {
    read_char(l);
    int position = l->position;
    int len = 0;
    while (!(l->ch == '"' || l->ch == 0)) {
        read_char(l);
        len++;
    }
    if (l->ch != '"') return NULL;
    return copy_string(l, position, len);
}

static char* read_identifier(Lexer* l) {
    int position = l->position;
    int len = 0;
    while (is_letter(l->ch)) {
        read_char(l);
        len++;
    }
    return copy_string(l, position, len);
}

static char* read_digit(Lexer* l, TokenType* t) {
    int position = l->position;
    int len = 0;

    bool (*valid) (char) = &is_digit;
    if (l->ch == '0') {
        char peek = peek_char(l);
        if (peek == 'x' || peek == 'X')
            valid = &is_hex_digit;
        else if (peek == 'b' || peek == 'B')
            valid = &is_binary_digit;
    }
    // skip 0x | 0b
    if (valid != &is_digit) {
        read_char(l);
        read_char(l);
        len = 2;
    }

    bool is_float = false;
    while (valid(l->ch)) {
        is_float = is_float || l->ch == '.';
        read_char(l);
        len++;
    }
    *t = is_float ? t_Float : t_Int;

    return copy_string(l, position, len);
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
            if (lit == NULL) die("malloc");
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
            if (lit == NULL) die("malloc");
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
        if (peek_char(l) == '/') {
            size_t position = l->read_position + 1;
            char ch;
            do {
                ch = l->input[++position];
            } while (position < l->input_len && ch != '\n');
            l->read_position = position;
            read_char(l);
            return lexer_next_token(l);
        } else {
            new_token(&tok, t_Slash, l->ch);
        }
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
    case ':':
        new_token(&tok, t_Colon, l->ch);
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
        return tok;
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
