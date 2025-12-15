#pragma once

// The modules contains the Parser, it constructs and AST (Program) from the
// source code using Pratt Parsing.

#include "ast.h"
#include "lexer.h"
#include "errors.h"
#include "token.h"

typedef struct Parser Parser;

typedef Node PrefixParseFn (Parser* p);
typedef Node InfixParseFn (Parser* p, Node left);

// All `parse_*` functions must return with `p->cur_token` in use or freed
struct Parser {
    Lexer l;
    Token cur_token;
    Token peek_token;

    ErrorBuffer errors;

    // Instead of a hashmap, each token contains a pointer to a parser function
    // for that token or NULL, which indicates it cannot be parsed.

    PrefixParseFn *prefix_parse_fns[t_Return + 1];
    InfixParseFn *infix_parse_fns[t_Return + 1];
};

void parser_init(Parser* p);
void parser_free(Parser* p);

// Create AST from [program], parsing until an error is encountered and stored
// in [p.errors].  On success, [p.errors.length] is 0.
Program parse(Parser* p, const char *program);
Program parse_(Parser* p, const char *program, uint64_t length);

void program_free(Program* p);

void print_parser_errors(Parser *p);

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
