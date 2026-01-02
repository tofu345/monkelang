#pragma once

// The modules contains the Parser, it constructs and AST (Program) from the
// source code using Pratt Parsing.

#include "ast.h"
#include "lexer.h"
#include "errors.h"
#include "token.h"

struct Parser;
typedef Node PrefixParseFn(struct Parser *);
typedef Node InfixParseFn(struct Parser *, Node left);

typedef struct {
    const char *message;
    bool allocated;
    Token token;
} ParseError;

BUFFER(ParseError, ParseError)

typedef struct Parser {
    Lexer *l;
    Token cur_token;
    Token peek_token;

    // ParseErrors of Program being parsed.
    ParseErrorBuffer errors;

    // Instead of a hashmap, each token contains a pointer to a parser function
    // for that token or NULL, which indicates it cannot be parsed.
    PrefixParseFn *prefix_parse_fns[t_Return + 1];
    InfixParseFn *infix_parse_fns[t_Return + 1];
} Parser;

// initialize prefix and infix parse functions.
void parser_init(Parser* p);

// parse AST in Lexer until an error is encountered and returned.
ParseErrorBuffer parse(Parser *p, Lexer *lexer, Program *program);

// print and free Parser errors.
void print_parse_errors(ParseErrorBuffer *, FILE *stream);
void free_parse_errors(ParseErrorBuffer *);

enum Precedence {
    p_Lowest = 1,
    p_Equals,
    p_LessGreater,
    p_Sum,
    p_Product,
    p_Prefix,
    p_Call,
    p_Index,
};
