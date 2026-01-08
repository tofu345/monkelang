#include "parser.h"
#include "ast.h"
#include "errors.h"
#include "utils.h"
#include "lexer.h"
#include "token.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(ParseError, ParseError)

#define INVALID (Node){ .obj = NULL }
#define IS_INVALID(n) (n.obj == NULL)

static void parse_error(Parser* p, char* format, ...);
static void parse_error_(Parser *p, const char* message);

static Node parse_statement(Parser* p);
static Node parse_expression(Parser* p, enum Precedence precedence);

// returns `NodeBuffer` with length of -1 on error.
static NodeBuffer parse_expression_list(Parser* p, TokenType end);

// must be called with [p.cur_token] at t_Lbrace '{'.
// returns NULL on error.
static BlockStatement *parse_block_statement(Parser* p);

// extend [tok] to include everything up to [p.cur_token].
static void extend_token(Parser *p, Token *tok) {
    tok->length = (p->cur_token.position + p->cur_token.length) - tok->position;
}

static void *
allocate(size_t size) {
    void* ptr = calloc(1, size);
    if (ptr == NULL) { die("parser - allocate:"); }
    return ptr;
}

struct {
    TokenType typ;
    enum Precedence precedence;
} const precedences[] = {
    { t_Eq, p_Equals },
    { t_Not_eq, p_Equals },
    { t_Lt, p_LessGreater },
    { t_Gt, p_LessGreater },
    { t_Plus, p_Sum },
    { t_Minus, p_Sum },
    { t_Slash, p_Product },
    { t_Asterisk, p_Product },
    { t_Lparen, p_Call },
    { t_Lbracket, p_Index },
};
const int precedences_len = sizeof(precedences) / sizeof(precedences[0]);

static enum Precedence
lookup_precedence(TokenType t) {
    for (int i = 0; i < precedences_len; i++) {
        if (precedences[i].typ == t) {
            return precedences[i].precedence;
        }
    }

    return p_Lowest;
}

static enum Precedence
peek_precedence(Parser* p) {
    return lookup_precedence(p->peek_token.type);
}

static enum Precedence
cur_precedence(Parser* p) {
    return lookup_precedence(p->cur_token.type);
}

static void
next_token(Parser* p) {
    p->cur_token = p->peek_token;
    p->peek_token = lexer_next_token(p->l);
}

static void
cur_tok_error(Parser* p, TokenType t) {
    parse_error(p,
            "expected token to be '%s', got '%s' instead",
            show_token_type(t), show_token_type(p->cur_token.type));
}

static void
peek_error(Parser* p, TokenType t) {
    parse_error(p,
            "expected next token to be '%s', got '%s' instead",
            show_token_type(t), show_token_type(p->peek_token.type));
}

inline static bool
cur_token_is(const Parser* p, TokenType t) {
    return p->cur_token.type == t;
}

inline static bool
peek_token_is(const Parser* p, TokenType t) {
    return p->peek_token.type == t;
}

// if [p.peek_token.typ] is [t] then call next_token() and return true,
// otherwise return false.
static bool
expect_peek(Parser* p, TokenType t) {
    if (peek_token_is(p, t)) {
        next_token(p);
        return true;
    } else {
        peek_error(p, t);
        return false;
    }
}

static Node
parse_expression(Parser* p, enum Precedence precedence) {
    PrefixParseFn *prefix = p->prefix_parse_fns[p->cur_token.type];
    if (prefix == NULL) {
        parse_error(p, "unexpected token '%s'", show_token_type(p->cur_token.type));
        return INVALID;
    }

    Node left_exp = prefix(p);
    if (IS_INVALID(left_exp)) {
        return INVALID;
    }

    while (!peek_token_is(p, t_Semicolon) && precedence < peek_precedence(p)) {
        InfixParseFn *infix = p->infix_parse_fns[p->peek_token.type];
        if (infix == NULL) {
            return left_exp;
        }

        next_token(p);

        left_exp = infix(p, left_exp);
        if (IS_INVALID(left_exp)) {
            return INVALID;
        }
    }

    return left_exp;
}

static Node
parse_identifier(Parser* p) {
    Identifier* id = allocate(sizeof(Identifier));
    id->tok = p->cur_token;
    return NODE(n_Identifier, id);
}

static Node
parse_float(Parser* p) {
    Token tok = p->cur_token;
    const char *end = tok.start + tok.length;
    char *endptr;

    errno = 0;
    double value = strtod(tok.start, &endptr);
    if (endptr != end || errno == ERANGE) {
        parse_error(p,
                "could not parse '%.*s' as float",
                LITERAL(p->cur_token));
        return INVALID;
    }

    FloatLiteral* fl = allocate(sizeof(FloatLiteral));
    fl->tok = p->cur_token;
    fl->value = value;
    return NODE(n_FloatLiteral, fl);
}

static Node
parse_integer(Parser* p) {
    Token tok = p->cur_token;
    const char *end = tok.start + tok.length;
    char* endptr;

    errno = 0;
    long value = strtol(tok.start, &endptr, 0);
    if (endptr != end) {
        parse_error(p,
                "could not parse '%.*s' as integer",
                LITERAL(p->cur_token));
        return INVALID;

    } else if (errno == ERANGE) {
        parse_error(p,
                "integer '%.*s' is out of range",
                LITERAL(p->cur_token));
        return INVALID;
    }

    IntegerLiteral* il = allocate(sizeof(IntegerLiteral));
    il->tok = p->cur_token;
    il->value = value;
    return NODE(n_IntegerLiteral, il);
}

static Node
parse_infix_expression(Parser* p, Node left) {
    InfixExpression* ie = allocate(sizeof(InfixExpression));
    ie->left = left;
    ie->op = p->cur_token;

    enum Precedence precedence = cur_precedence(p);
    next_token(p);
    ie->right = parse_expression(p, precedence);
    if (IS_INVALID(ie->right)) {
        node_free(left);
        free(ie);
        return INVALID;
    }

    return NODE(n_InfixExpression, ie);
}

static Node
parse_prefix_expression(Parser* p) {
    PrefixExpression* pe = allocate(sizeof(PrefixExpression));
    pe->op = p->cur_token;

    next_token(p);

    pe->right = parse_expression(p, p_Prefix);
    if (IS_INVALID(pe->right)) {
        free(pe);
        return INVALID;
    }

    return NODE(n_PrefixExpression, pe);
}

static Node
parse_boolean(Parser* p) {
    BooleanLiteral* b = allocate(sizeof(BooleanLiteral));
    b->tok = p->cur_token;
    b->value = p->cur_token.type == t_True;
    return NODE(n_BooleanLiteral, b);
}

static Node
parse_grouped_expression(Parser* p) {
    next_token(p);
    Node n = parse_expression(p, p_Lowest);
    if (IS_INVALID(n) || !expect_peek(p, t_Rparen)) {
        node_free(n);
        return INVALID;
    }
    return n;
}

static BlockStatement *
parse_block_statement(Parser* p) {
    BlockStatement *bs = allocate(sizeof(BlockStatement));
    bs->tok = p->cur_token;

    next_token(p);

    while (!cur_token_is(p, t_Rbrace) && !cur_token_is(p, t_Eof)) {
        Node stmt = parse_statement(p);
        if (IS_INVALID(stmt)) {
            free_block_statement(bs);
            return NULL;
        }

        NodeBufferPush(&bs->stmts, stmt);
        next_token(p);
    }

    if (!cur_token_is(p, t_Rbrace)) {
        cur_tok_error(p, t_Rbrace);
        free_block_statement(bs);
        return NULL;
    }

    return bs;
}

// must be called with [p.cur_token] on t_Lparen '('.
// returns -1 on error.
static int
parse_function_parameters(Parser* p, FunctionLiteral* fl) {
    next_token(p);

    if (cur_token_is(p, t_Rparen)) {
        return 0;
    }

    if (p->cur_token.type != t_Ident) {
        cur_tok_error(p, t_Ident);
        return -1;
    }

    Identifier* ident = parse_identifier(p).obj;
    BufferPush(&fl->params, ident);

    while (peek_token_is(p, t_Comma)) {
        next_token(p);

        if (!expect_peek(p, t_Ident)) { return -1; }

        ident = parse_identifier(p).obj;
        BufferPush(&fl->params, ident);
    }

    if (!expect_peek(p, t_Rparen)) { return -1; }

    return 0;
}

static Node
parse_function_literal(Parser* p) {
    FunctionLiteral* fl = allocate(sizeof(FunctionLiteral));
    fl->tok = p->cur_token;

    if (!expect_peek(p, t_Lparen)) { goto invalid; }

    int err = parse_function_parameters(p, fl);
    if (err == -1 || !expect_peek(p, t_Lbrace)) { goto invalid; }

    fl->body = parse_block_statement(p);
    if (!fl->body) { goto invalid; }

    return NODE(n_FunctionLiteral, fl);

invalid:
    free_function_literal(fl);
    return INVALID;
}

static Node
parse_string_literal(Parser* p) {
    if (p->cur_token.start[p->cur_token.length] != '\"') {
        // make token point to opening quote
        --p->cur_token.start;
        --p->cur_token.position;
        p->cur_token.length = 1;
        parse_error_(p, "missing closing '\"'");
        return INVALID;
    }

    StringLiteral* sl = allocate(sizeof(StringLiteral));
    sl->tok = p->cur_token;
    return NODE(n_StringLiteral, sl);
}

static Node
parse_array_literal(Parser* p) {
    ArrayLiteral* al = allocate(sizeof(ArrayLiteral));
    al->tok = p->cur_token;
    al->elements = parse_expression_list(p, t_Rbracket);
    if (al->elements.length == -1) {
        free(al);
        return INVALID;
    }
    return NODE(n_ArrayLiteral, al);
}

static Node
parse_table_literal(Parser* p) {
    TableLiteral* tbl = allocate(sizeof(TableLiteral));
    tbl->tok = p->cur_token;

    while (!peek_token_is(p, t_Rbrace)) {
        next_token(p);

        Node key = parse_expression(p, p_Lowest);
        if (IS_INVALID(key)) { goto invalid; }

        TokenType peek = p->peek_token.type;
        if (key.typ == n_Identifier && (peek == t_Comma || peek == t_Rbrace))
        {
            // pair with identifier name and value.

            StringLiteral* str = allocate(sizeof(StringLiteral));
            str->tok = *node_token(key);
            Node str_key = NODE(n_StringLiteral, str);

            PairBufferPush(&tbl->pairs, (Pair){ str_key, key });
            if (peek == t_Comma) {
                next_token(p);
            }
            continue;
        }

        if (!expect_peek(p, t_Colon)) {
            node_free(key);
            goto invalid;
        }

        next_token(p);

        Node value = parse_expression(p, p_Lowest);
        if (IS_INVALID(value)) {
            node_free(key);
            goto invalid;
        }

        PairBufferPush(&tbl->pairs, (Pair){ key, value });

        if (peek_token_is(p, t_Rbrace)) {
            break;
        } else if (!expect_peek(p, t_Comma)) {
            goto invalid;
        }
    }

    if (!expect_peek(p, t_Rbrace)) { goto invalid; }

    return NODE(n_TableLiteral, tbl);

invalid:
    free_table_literal(tbl);
    return INVALID;
}

static Node
parse_call_expression(Parser* p, Node function) {
    Token tok = *node_token(function);

    NodeBuffer args = parse_expression_list(p, t_Rparen);
    if (args.length == -1) {
        node_free(function);
        return INVALID;
    }
    extend_token(p, &tok);

    CallExpression* ce = allocate(sizeof(CallExpression));
    ce->function = function;
    ce->args = args;
    ce->tok = tok;
    return NODE(n_CallExpression, ce);
}

static Node
parse_index_expression(Parser* p, Node left) {
    Token tok = *node_token(left);

    next_token(p);

    Node index = parse_expression(p, p_Lowest);
    if (IS_INVALID(index) || !expect_peek(p, t_Rbracket)) {
        node_free(index);
        node_free(left);
        return INVALID;
    }
    extend_token(p, &tok);

    IndexExpression* ie = allocate(sizeof(IndexExpression));
    ie->left = left;
    ie->index = index;
    ie->tok = tok;
    return NODE(n_IndexExpression, ie);
}

// must be called with p.cur_token at the start token, e.g '('
static NodeBuffer
parse_expression_list(Parser* p, TokenType end) {
    NodeBuffer list = {0};

    if (peek_token_is(p, end)) {
        next_token(p);
        return list;
    }

    next_token(p);

    Node exp = parse_expression(p, p_Lowest);
    if (IS_INVALID(exp)) { goto invalid; }

    NodeBufferPush(&list, exp);

    while (peek_token_is(p, t_Comma)) {
        next_token(p);
        next_token(p);

        Node exp = parse_expression(p, p_Lowest);
        if (IS_INVALID(exp)) { goto invalid; }

        NodeBufferPush(&list, exp);
    }

    if (!expect_peek(p, end)) { goto invalid; }

    return list;

invalid:
    for (int i = 0; i < list.length; i++) {
        node_free(list.data[i]);
    }
    free(list.data);
    list.data = NULL;
    list.length = -1;
    return list;
}

static Node
parse_if_expression(Parser* p) {
    IfExpression* ie = allocate(sizeof(IfExpression));
    ie->tok = p->cur_token;

    if (!expect_peek(p, t_Lparen)) { goto invalid; }

    next_token(p);

    if (cur_token_is(p, t_Rparen)) {
        parse_error_(p, "empty if statement");
        goto invalid;
    }

    ie->condition = parse_expression(p, p_Lowest);
    if (IS_INVALID(ie->condition)
            || !expect_peek(p, t_Rparen)
            || !expect_peek(p, t_Lbrace)) {
        goto invalid;
    }

    ie->consequence = parse_block_statement(p);
    if (!ie->consequence) { goto invalid; }

    if (peek_token_is(p, t_Else)) {
        next_token(p);

        if (!expect_peek(p, t_Lbrace)) { goto invalid; }

        ie->alternative = parse_block_statement(p);
        if (!ie->alternative) { goto invalid; }
    }

    return NODE(n_IfExpression, ie);

invalid:
    free_if_expression(ie);
    return INVALID;
}

static Node
parse_nothing_literal(Parser *p) {
    NothingLiteral* nl = allocate(sizeof(NothingLiteral));
    nl->tok = p->cur_token;
    return NODE(n_NothingLiteral, nl);
}

static Node
parse_require_expression(Parser *p) {
    Token tok = p->cur_token;
    next_token(p);

    NodeBuffer args = parse_expression_list(p, t_Rparen);
    if (args.length == -1) { return INVALID; }
    extend_token(p, &tok);

    RequireExpression* re = allocate(sizeof(RequireExpression));
    re->tok = tok;
    re->args = args;
    return NODE(n_RequireExpression, re);
}

inline static int
_parse_let_statement(Parser *p, LetStatement *ls) {
    if (!expect_peek(p, t_Ident)) { return -1; }

    Identifier *name = parse_identifier(p).obj;
    BufferPush(&ls->names, name);

    if (!peek_token_is(p, t_Assign)) {
        NodeBufferPush(&ls->values, (Node){0});
        return 0;
    }

    // (x2) next_token() to skip t_Assign
    p->cur_token = lexer_next_token(p->l);
    p->peek_token = lexer_next_token(p->l);

    Node value = parse_expression(p, p_Lowest);
    if (IS_INVALID(value)) { return -1; }
    NodeBufferPush(&ls->values, value);

    if (value.typ == n_FunctionLiteral) {
        FunctionLiteral *fl = value.obj;
        fl->name = name;
    }
    return 0;
}

static Node
parse_let_statement(Parser* p) {
    LetStatement* stmt = allocate(sizeof(LetStatement));
    stmt->tok = p->cur_token;

    if (_parse_let_statement(p, stmt) == -1) { goto invalid; }

    while (peek_token_is(p, t_Comma)) {
        next_token(p);
        if (_parse_let_statement(p, stmt) == -1) {
            goto invalid;
        }
    }

    return NODE(n_LetStatement, stmt);

invalid:
    node_free(NODE(n_LetStatement, stmt));
    return INVALID;
}

static Node
parse_return_statement(Parser* p) {
    ReturnStatement* stmt = allocate(sizeof(ReturnStatement));
    stmt->tok = p->cur_token;

    // the expression after the 'return' can be parsed
    if (p->prefix_parse_fns[p->peek_token.type]) {
        next_token(p);

        stmt->return_value = parse_expression(p, p_Lowest);
        if (IS_INVALID(stmt->return_value)) { goto invalid; }
    }

    return NODE(n_ReturnStatement, stmt);

invalid:
    node_free(NODE(n_ReturnStatement, stmt));
    return INVALID;
}

static Node
parse_assignment(Parser *p, Node left) {
    Token tok = p->peek_token;

    // (x2) next_token() to skip t_Assign
    p->cur_token = lexer_next_token(p->l);
    p->peek_token = lexer_next_token(p->l);

    Node right = parse_expression(p, p_Lowest);
    if (IS_INVALID(right)) { goto invalid; }

    if (left.typ == n_Identifier && right.typ == n_FunctionLiteral) {
        FunctionLiteral *fl = right.obj;
        fl->name = left.obj;
    }

    Assignment *as = allocate(sizeof(Assignment));
    as->left = left;
    as->tok = tok;
    as->right = right;
    return NODE(n_Assignment, as);

invalid:
    node_free(left);
    return INVALID;
}

static Node
parse_operator_assignment(Parser *p, Node left) {
    Token tok = p->peek_token;

    // (x2) next_token() to skip t_(operator)_Assign
    p->cur_token = lexer_next_token(p->l);
    p->peek_token = lexer_next_token(p->l);

    Node right = parse_expression(p, p_Lowest);
    if (IS_INVALID(right)) { goto invalid; }

    OperatorAssignment *stmt = allocate(sizeof(OperatorAssignment));
    stmt->op = tok;
    stmt->left = left;
    stmt->right = right;

    return NODE(n_OperatorAssignment, stmt);

invalid:
    node_free(left);
    return INVALID;
}

static Node
parse_expression_statement(__attribute__ ((unused)) Parser* p, Node left) {
    ExpressionStatement *es = allocate(sizeof(ExpressionStatement));
    es->tok = *node_token(left);
    es->expression = left;
    return NODE(n_ExpressionStatement, es);
}

static Node
parse_break_statement(Parser *p) {
    BreakStatement *bs = allocate(sizeof(BreakStatement));
    bs->tok = p->cur_token;
    return NODE(n_BreakStatement, bs);
}

static Node
parse_continue_statement(Parser *p) {
    ContinueStatement *cs = allocate(sizeof(ContinueStatement));
    cs->tok = p->cur_token;
    return NODE(n_ContinueStatement, cs);
}

static Node
parse_for_statement(Parser *p) {
    LoopStatement *loop = allocate(sizeof(LoopStatement));
    loop->tok = p->cur_token;

    // for ( ...
    if (!expect_peek(p, t_Lparen)) { goto invalid; }
    next_token(p);

    // for ( [start] ; ...
    if (!cur_token_is(p, t_Semicolon)) {
        loop->start = parse_statement(p);
        if (IS_INVALID(loop->start)) { goto invalid; }
    }
    next_token(p);

    // ...; [condition] ; ...
    if (!cur_token_is(p, t_Semicolon)) {
        loop->condition = parse_expression(p, p_Lowest);
        if (IS_INVALID(loop->condition)
                || !expect_peek(p, t_Semicolon)) { goto invalid; }
    }

    // ...; [update] ) {
    if (!peek_token_is(p, t_Rparen)) {
        next_token(p);
        loop->update = parse_statement(p);
        if (IS_INVALID(loop->update)) { goto invalid; }
    }

    // ) {
    if (!expect_peek(p, t_Rparen)) { goto invalid; }
    next_token(p);

    loop->body = parse_block_statement(p);
    if (!loop->body) { goto invalid; }

    return NODE(n_LoopStatement, loop);

invalid:
    free_loop_statement(loop);
    return INVALID;
}

static Node
parse_while_statement(Parser *p) {
    LoopStatement *loop = allocate(sizeof(LoopStatement));
    loop->tok = p->cur_token;

    if (!expect_peek(p, t_Lparen)) { goto invalid; }
    next_token(p);

    loop->condition = parse_expression(p, p_Lowest);
    if (IS_INVALID(loop->condition)) { goto invalid; }

    if (!expect_peek(p, t_Rparen)) { goto invalid; }
    next_token(p);

    loop->body = parse_block_statement(p);
    if (!loop->body) { goto invalid; }

    return NODE(n_LoopStatement, loop);

invalid:
    free_loop_statement(loop);
    return INVALID;
}

// on failure, return Node with `n.obj` == NULL
inline static Node
parse_statement_(Parser *p) {
    switch (p->cur_token.type) {
    case t_Let:
        return parse_let_statement(p);
    case t_Return:
        return parse_return_statement(p);
    case t_For:
        return parse_for_statement(p);
    case t_While:
        return parse_while_statement(p);
    case t_Break:
        return parse_break_statement(p);
    case t_Continue:
        return parse_continue_statement(p);
    case t_Illegal:
        parse_error(p,
                "illegal character '%.*s'",
                p->cur_token.length, p->cur_token.start);
        return INVALID;
    default:
        {
            Node left = parse_expression(p, p_Lowest);
            if (IS_INVALID(left)) {
                return INVALID;
            }

            TokenType peek = p->peek_token.type;

            if (peek == t_Assign) {
                return parse_assignment(p, left);

            } else if (peek >= t_Add_Assign && peek <= t_Div_Assign) {
                return parse_operator_assignment(p, left);

            } else {
                return parse_expression_statement(p, left);
            }
        }
    }
}

// inspired by https://youtu.be/6CV8HyqJ_9w?si=XJWE1RCAcCOsvzVp
//
// return `true` if [p.peek_token] is a `t_Semicolon` or on a new line,
// with some exceptions.
inline static bool
peek_semicolon_or_newline(Parser *p) {
    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
        return true;

    } else if (p->peek_token.line == p->cur_token.line) {
        switch (p->peek_token.type) {
            case t_Eof:
            case t_Rbracket:
            case t_Rbrace:
            case t_Rparen:
                return true;
            default:
                next_token(p); // make [p.peek_token] [p.cur_token]
                parse_error_(p,
                        "this statement must be on a new line or come after a semicolon");
                return false;
        }
    }

    return true;
}

static Node
parse_statement(Parser* p) {
    Node stmt = parse_statement_(p);
    if (stmt.obj && !peek_semicolon_or_newline(p)) {
        node_free(stmt);
        return INVALID;
    }
    return stmt;
}

void parser_init(Parser* p) {
    memset(p, 0, sizeof(Parser));

    p->prefix_parse_fns[t_Ident] = parse_identifier;
    p->prefix_parse_fns[t_Integer] = parse_integer;
    p->prefix_parse_fns[t_Float] = parse_float;
    p->prefix_parse_fns[t_Bang] = parse_prefix_expression;
    p->prefix_parse_fns[t_Minus] = parse_prefix_expression;
    p->prefix_parse_fns[t_True] = parse_boolean;
    p->prefix_parse_fns[t_False] = parse_boolean;
    p->prefix_parse_fns[t_Lparen] = parse_grouped_expression;
    p->prefix_parse_fns[t_If] = parse_if_expression;
    p->prefix_parse_fns[t_Function] = parse_function_literal;
    p->prefix_parse_fns[t_String] = parse_string_literal;
    p->prefix_parse_fns[t_Lbracket] = parse_array_literal;
    p->prefix_parse_fns[t_Lbrace] = parse_table_literal;
    p->prefix_parse_fns[t_Nothing] = parse_nothing_literal;
    p->prefix_parse_fns[t_Require] = parse_require_expression;

    p->infix_parse_fns[t_Plus] = parse_infix_expression;
    p->infix_parse_fns[t_Minus] = parse_infix_expression;
    p->infix_parse_fns[t_Slash] = parse_infix_expression;
    p->infix_parse_fns[t_Asterisk] = parse_infix_expression;
    p->infix_parse_fns[t_Eq] = parse_infix_expression;
    p->infix_parse_fns[t_Not_eq] = parse_infix_expression;
    p->infix_parse_fns[t_Lt] = parse_infix_expression;
    p->infix_parse_fns[t_Gt] = parse_infix_expression;
    p->infix_parse_fns[t_Lparen] = parse_call_expression;
    p->infix_parse_fns[t_Lbracket] = parse_index_expression;
}

ParseErrorBuffer parse(Parser *p, Lexer *lexer, Program *program) {
    p->l = lexer;
    p->cur_token = lexer_next_token(p->l);
    p->peek_token = lexer_next_token(p->l);

    while (p->cur_token.type != t_Eof) {
        Node stmt = parse_statement(p);
        if (IS_INVALID(stmt)) {
            // exit immediately on error
            break;
        }

        NodeBufferPush(&program->stmts, stmt);
        next_token(p);
    }

    ParseErrorBuffer errors = p->errors;
    p->errors = (ParseErrorBuffer){0};
    return errors;
}

static void
parse_error_(Parser *p, const char* message) {
    ParseErrorBufferPush(&p->errors, (ParseError) {
        .message = message,
        .allocated = false,
        .token = p->cur_token,
    });
}

static void
parse_error(Parser* p, char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    if (vasprintf(&msg, format, args) == -1) {
        die("parse_error vasprintf:");
    }
    va_end(args);

    ParseErrorBufferPush(&p->errors, (ParseError) {
        .message = msg,
        .allocated = true,
        .token = p->cur_token,
    });
}

void print_parse_errors(ParseErrorBuffer *errors, FILE *stream) {
    for (int i = 0; i < errors->length; i++) {
        ParseError err = errors->data[i];

        highlight_token(&err.token, 2, stream);
        fprintf(stream, "%s\n", err.message);

        if (err.allocated) {
            free((char *) err.message);
        }
    }
    free(errors->data);
    *errors = (ParseErrorBuffer){0};
}

void free_parse_errors(ParseErrorBuffer *errors) {
    for (int i = 0; i < errors->length; i++) {
        ParseError err = errors->data[i];
        if (err.allocated) {
            free((char *) err.message);
        }
    }
    free(errors->data);
    *errors = (ParseErrorBuffer){0};
}
