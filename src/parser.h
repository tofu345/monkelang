#pragma once

#include "ast.h"
#include "lexer.h"
#include "token.h"

#define MAX_ERRORS 16

typedef struct Parser Parser;

typedef Node (*PrefixParseFn) (Parser* p);
typedef Node (*InfixParseFn) (Parser* p, Node left);

// All `parse_*` functions must return with `p->cur_token`
// in use or freed
struct Parser {
    Lexer* l;
    Token cur_token;
    Token peek_token;

    StringBuffer errors;

    // TODO: change order of tokens and reduce array size.
    PrefixParseFn prefix_parse_fns[t_Return];
    InfixParseFn infix_parse_fns[t_Return];
};

void parser_init(Parser* p, Lexer* l);
void parser_destroy(Parser* p);

Program parse_program(Parser* p);
void program_destroy(Program* p);

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
