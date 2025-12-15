#include "ast.h"
#include "errors.h"
#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INVALID (Node){ .obj = NULL }
#define IS_INVALID(n) (n.obj == NULL)

// create [ParserError] with [p.cur_token]
static void parser_error(Parser* p, char* format, ...);

static Node parse_statement(Parser* p);
static Node parse_expression(Parser* p, enum Precedence precedence);
static NodeBuffer parse_expression_list(Parser* p, TokenType end);

static void *
allocate(size_t size) {
    void* ptr = calloc(1, size);
    if (ptr == NULL) die("parser: malloc");
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

static enum Precedence
lookup_precedence(TokenType t) {
    static const size_t precedences_len =
        sizeof(precedences) / sizeof(precedences[0]);

    for (size_t i = 0; i < precedences_len; i++) {
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
    p->peek_token = lexer_next_token(&p->l);
}

static void
cur_tok_error(Parser* p, TokenType t) {
    parser_error(p,
            "expected token to be '%s', got '%s' instead",
            show_token_type(t), show_token_type(p->cur_token.type));
}

static void
peek_error(Parser* p, TokenType t) {
    parser_error(p,
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

// if peek_token.typ is [t] call [next_token] and return true,
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

static void
peek_semicolon(Parser *p) {
    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
    }
}

static void
no_prefix_parse_error(Parser* p, TokenType t) {
    parser_error(p, "unexpected token '%s'", show_token_type(t));
}

static Node
parse_expression(Parser* p, enum Precedence precedence) {
    PrefixParseFn *prefix = p->prefix_parse_fns[p->cur_token.type];
    if (prefix == NULL) {
        no_prefix_parse_error(p, p->cur_token.type);
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

    errno = 0; // must reset errno
    double value = strtod(tok.start, &endptr);
    if (endptr != end || errno == ERANGE) {
        parser_error(p,
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

    errno = 0; // must reset errno
    long value = strtol(tok.start, &endptr, 0);
    if (endptr != end) {
        parser_error(p,
                "could not parse '%.*s' as integer",
                LITERAL(p->cur_token));
        return INVALID;

    } else if (errno == ERANGE) {
        parser_error(p,
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
    ie->tok = p->cur_token;

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
    pe->tok = p->cur_token;

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

static BlockStatement*
parse_block_statement(Parser* p) {
    BlockStatement* bs = allocate(sizeof(BlockStatement));
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

// returns -1 on err
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
    ParamBufferPush(&fl->params, ident);

    while (peek_token_is(p, t_Comma)) {
        next_token(p);

        if (!expect_peek(p, t_Ident))
            return -1;

        ident = parse_identifier(p).obj;
        ParamBufferPush(&fl->params, ident);
    }

    if (!expect_peek(p, t_Rparen))
        return -1;

    return 0;
}

static Node
parse_function_literal(Parser* p) {
    FunctionLiteral* fl = allocate(sizeof(FunctionLiteral));
    fl->tok = p->cur_token;
    fl->name = NULL;

    if (!expect_peek(p, t_Lparen)) {
        free(fl);
        return INVALID;
    }

    int err = parse_function_parameters(p, fl);
    if (err == -1 || !expect_peek(p, t_Lbrace)) {
        fl->body = NULL;
        free_function_literal(fl);
        return INVALID;
    }

    fl->body = parse_block_statement(p);
    if (fl->body == NULL) {
        free_function_literal(fl);
        return INVALID;
    }

    return NODE(n_FunctionLiteral, fl);
}

static Node
parse_string_literal(Parser* p) {
    if (p->cur_token.start[p->cur_token.length] != '\"') {
        // make token point to opening quote
        --p->cur_token.start;
        --p->cur_token.position;
        p->cur_token.length = 1;

        parser_error(p, "missing closing '\"'");
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
    TableLiteral* hl = allocate(sizeof(TableLiteral));
    hl->tok = p->cur_token;

    while (!peek_token_is(p, t_Rbrace)) {
        next_token(p);

        Node key = parse_expression(p, p_Lowest);
        if (IS_INVALID(key)) {
            free_table_literal(hl);
            return INVALID;
        }

        if (!expect_peek(p, t_Colon)) {
            node_free(key);
            free_table_literal(hl);
            return INVALID;
        }

        next_token(p);

        Node value = parse_expression(p, p_Lowest);
        if (IS_INVALID(value)) {
            node_free(key);
            free_table_literal(hl);
            return INVALID;
        }

        PairBufferPush(&hl->pairs, (Pair){ key, value });

        if (peek_token_is(p, t_Rbrace)) {
            break;

        } else if (!expect_peek(p, t_Comma)) {
            free_table_literal(hl);
            return INVALID;
        }
    }

    if (!expect_peek(p, t_Rbrace)) {
        free_table_literal(hl);
        return INVALID;
    }

    return NODE(n_TableLiteral, hl);
}

static Node
parse_call_expression(Parser* p, Node function) {
    Token tok = *node_token(function);

    NodeBuffer args = parse_expression_list(p, t_Rparen);
    if (args.length == -1) {
        node_free(function);
        return INVALID;
    }

    CallExpression* ce = allocate(sizeof(CallExpression));
    ce->function = function;
    ce->args = args;

    tok.length = (p->cur_token.position + p->cur_token.length) - tok.position;
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

    tok.length = (p->cur_token.position + p->cur_token.length) - tok.position;

    IndexExpression* ie = allocate(sizeof(IndexExpression));
    ie->left = left;
    ie->index = index;
    ie->tok = tok;
    return NODE(n_IndexExpression, ie);
}

static NodeBuffer
__free_elements_in(NodeBuffer buf) {
    if (buf.data != NULL) {
        for (int i = 0; i < buf.length; i++) {
            node_free(buf.data[i]);
        }
        free(buf.data);
        buf.data = NULL;
    }
    buf.length = -1;
    return buf;
}

// [NodeBuffer.length] is -1 on err.
static NodeBuffer
parse_expression_list(Parser* p, TokenType end) {
    NodeBuffer list = {0};

    if (peek_token_is(p, end)) {
        next_token(p);
        return list;
    }

    next_token(p);

    Node exp = parse_expression(p, p_Lowest);
    if (IS_INVALID(exp))
        return __free_elements_in(list);

    NodeBufferPush(&list, exp);

    while (peek_token_is(p, t_Comma)) {
        next_token(p);
        next_token(p);

        Node exp = parse_expression(p, p_Lowest);
        if (IS_INVALID(exp))
            return __free_elements_in(list);

        NodeBufferPush(&list, exp);
    }

    if (!expect_peek(p, end))
        return __free_elements_in(list);

    return list;
}

static Node
parse_if_expression(Parser* p) {
    Token tok = p->cur_token;

    if (!expect_peek(p, t_Lparen)) {
        return INVALID;
    }

    next_token(p);

    if (cur_token_is(p, t_Rparen)) {
        parser_error(p, "empty if statement");
        return INVALID;
    }

    Node condition = parse_expression(p, p_Lowest);
    if (IS_INVALID(condition)
            || !expect_peek(p, t_Rparen)
            || !expect_peek(p, t_Lbrace)) {

        node_free(condition);
        return INVALID;
    }

    BlockStatement *consequence = parse_block_statement(p);
    if (consequence == NULL) {
        node_free(condition);
        return INVALID;
    }

    IfExpression* ie = allocate(sizeof(IfExpression));
    *ie = (IfExpression) {
        .tok = tok,
        .condition = condition,
        .consequence = consequence
    };

    if (peek_token_is(p, t_Else)) {
        next_token(p);

        if (!expect_peek(p, t_Lbrace)) {
            node_free(NODE(n_IfExpression, ie));
            return INVALID;
        }

        ie->alternative = parse_block_statement(p);
        if (ie->alternative == NULL) {
            node_free(NODE(n_IfExpression, ie));
            return INVALID;
        }
    }

    return NODE(n_IfExpression, ie);
}

static Node
parse_null_literal(Parser *p) {
    NullLiteral* nl = allocate(sizeof(NullLiteral));
    nl->tok = p->cur_token;
    return NODE(n_NullLiteral, nl);
}

static Node
parse_let_statement(Parser* p) {
    LetStatement* stmt = allocate(sizeof(LetStatement));
    stmt->tok = p->cur_token;

    if (!expect_peek(p, t_Ident)) {
        free(stmt);
        return INVALID;
    }

    stmt->name = parse_identifier(p).obj;

    if (!expect_peek(p, t_Assign)) {
        free(stmt->name);
        free(stmt);
        return INVALID;
    }
    next_token(p);

    stmt->value = parse_expression(p, p_Lowest);
    if (IS_INVALID(stmt->value)) {
        node_free(NODE(n_LetStatement, stmt));
        return INVALID;
    }

    if (stmt->value.typ == n_FunctionLiteral) {
        FunctionLiteral *fl = stmt->value.obj;
        fl->name = stmt->name;
    }

    peek_semicolon(p);
    return NODE(n_LetStatement, stmt);
}

static Node
parse_return_statement(Parser* p) {
    ReturnStatement* stmt = allocate(sizeof(ReturnStatement));
    stmt->tok = p->cur_token;

    // empty return statement if the expression after 'return' cannot be
    // parsed.
    if (p->prefix_parse_fns[p->peek_token.type]) {
        next_token(p);

        stmt->return_value = parse_expression(p, p_Lowest);
        if (IS_INVALID(stmt->return_value)) {
            free(stmt);
            return INVALID;
        }
    }

    peek_semicolon(p);
    return NODE(n_ReturnStatement, stmt);
}

static Node
parse_assignment(Parser *p, Node left) {
    Token tok = p->peek_token;

    // (x2) next_token() to skip t_Assign
    p->cur_token = lexer_next_token(&p->l);
    p->peek_token = lexer_next_token(&p->l);

    Node right = parse_expression(p, p_Lowest);
    if (IS_INVALID(right)) {
        node_free(left);
        return INVALID;
    }

    if (left.typ == n_Identifier && right.typ == n_FunctionLiteral) {
        FunctionLiteral *fl = right.obj;
        fl->name = left.obj;
    }

    Assignment *as = allocate(sizeof(Assignment));
    as->left = left;
    as->tok = tok;
    as->right = right;
    peek_semicolon(p);
    return NODE(n_Assignment, as);
}

static Node
parse_operator_assignment(Parser *p, Node left) {
    Token tok = p->peek_token;

    // (x2) next_token() to skip t_(operator)_Assign
    p->cur_token = lexer_next_token(&p->l);
    p->peek_token = lexer_next_token(&p->l);

    Node right = parse_expression(p, p_Lowest);
    if (IS_INVALID(right)) {
        node_free(left);
        return INVALID;
    }

    OperatorAssignment *stmt = allocate(sizeof(OperatorAssignment));
    stmt->tok = tok;
    stmt->left = left;
    stmt->right = right;
    peek_semicolon(p);
    return NODE(n_OperatorAssignment, stmt);
}

static Node
parse_expression_statement(__attribute__ ((unused)) Parser* p, Node left) {
    ExpressionStatement *es = allocate(sizeof(ExpressionStatement));
    es->tok = *node_token(left);
    es->expression = left;
    peek_semicolon(p);
    return NODE(n_ExpressionStatement, es);
}

static Node
parse_for_statement(Parser *p) {
    ForStatement *fs = allocate(sizeof(ForStatement));
    fs->tok = p->cur_token;

    // for ( ...
    if (!expect_peek(p, t_Lparen)) { goto fail; }
    next_token(p);

    // for ( [optional init] ; ...
    if (!cur_token_is(p, t_Semicolon)) {
        fs->init = parse_statement(p);
        if (IS_INVALID(fs->init)) { goto fail; }
    }
    next_token(p);

    // ...; [optional condition] ; ...
    if (!cur_token_is(p, t_Semicolon)) {
        fs->condition = parse_expression(p, p_Lowest);
        if (IS_INVALID(fs->condition)
                || !expect_peek(p, t_Semicolon)) { goto fail; }
    }

    // ...; [optional update] ) {
    if (!peek_token_is(p, t_Rparen)) {
        next_token(p);
        fs->update = parse_statement(p);
        if (IS_INVALID(fs->update)) { goto fail; }
    }
    if (!expect_peek(p, t_Rparen)) { goto fail; }
    next_token(p);

    fs->body = parse_block_statement(p);
    if (fs->body == NULL) { goto fail; }

    return NODE(n_ForStatement, fs);

fail:
    free_for_statement(fs);
    return INVALID;
}

// on failure, return Node with `n.obj` == NULL
static Node
parse_statement(Parser* p) {
    switch (p->cur_token.type) {
    case t_Let:
        return parse_let_statement(p);
    case t_Return:
        return parse_return_statement(p);
    case t_For:
        return parse_for_statement(p);
    case t_Illegal:
        parser_error(p,
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

void parser_init(Parser* p) {
    memset(p, 0, sizeof(Parser));

    lexer_init(&p->l, NULL);

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
    p->prefix_parse_fns[t_Null] = parse_null_literal;

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

void parser_free(Parser* p) {
    for (int i = 0; i < p->errors.length; i++) {
        free_error(p->errors.data[i]);
    }
    free(p->errors.data);
}

inline static Program
_parse(Parser *p, const char *program, uint64_t length) {
    lexer_with(&p->l, program, length);
    p->cur_token = lexer_next_token(&p->l);
    p->peek_token = lexer_next_token(&p->l);

    Program prog = {0};

    while (p->cur_token.type != t_Eof) {
        Node stmt = parse_statement(p);
        if (IS_INVALID(stmt)) {
            return prog;
        }

        NodeBufferPush(&prog.stmts, stmt);
        next_token(p);
    }
    return prog;
}

Program parse(Parser* p, const char *program) {
    return _parse(p, program, strlen(program));
}

Program parse_(Parser* p, const char *program, uint64_t length) {
    return _parse(p, program, length);
}

void program_free(Program* p) {
    for (int i = 0; i < p->stmts.length; i++) {
        node_free(p->stmts.data[i]);
    }
    free(p->stmts.data);
}

static void
parser_error(Parser* p, char* format, ...) {
    va_list args;
    va_start(args, format);
    error err = verrorf(format, args);
    va_end(args);

    error_with(&err, &p->cur_token);
    ErrorBufferPush(&p->errors, err);
}
