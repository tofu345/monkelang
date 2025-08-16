#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct {
    Lexer* l;
    char** errors;
    size_t errors_len;
    size_t errors_cap;

    Token cur_token;
    Token peek_token;
} Parser;

Parser parser_new(Lexer* l);

void parser_destroy(Parser* p);

// returns a program with `cap` 0 on fail
Program parse_program(Parser* p);

#endif
