#include "ast.h"
#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Node parse_statement(Parser* p);
static Node parse_expression(Parser* p, enum Precedence precedence);
static NodeBuffer parse_expression_list(Parser* p, TokenType end);

static void *
allocate(size_t size) {
    void* ptr = malloc(size);
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
parser_error(Parser* p, char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    if (vasprintf(&msg, format, args) == -1) die("parser_error");
    ErrorBufferPush(&p->errors, msg);
    va_end(args);
}

static void
next_token(Parser* p) {
    p->cur_token = p->peek_token;
    p->peek_token = lexer_next_token(p->l);
}

static void
cur_tok_error(Parser* p, TokenType t) {
    parser_error(p,
            ":%d,%d: expected token to be '%s', got '%s' instead",
            p->cur_token.line, p->cur_token.col,
            show_token_type(t), show_token_type(p->peek_token.type));
}

static void
peek_error(Parser* p, TokenType t) {
    parser_error(p,
            ":%d,%d: expected next token to be '%s', got '%s' instead",
            p->cur_token.line, p->cur_token.col,
            show_token_type(t), show_token_type(p->peek_token.type));
}

static bool
cur_token_is(const Parser* p, TokenType t) {
    return p->cur_token.type == t;
}

inline static bool
peek_token_is(const Parser* p, TokenType t) {
    return p->peek_token.type == t;
}

// if peek_token.typ is not [t] return false, else return true and call
// [next_token].
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
no_prefix_parse_error(Parser* p, TokenType t) {
    parser_error(p,
            ":%d,%d: unexpected token '%s'",
            p->cur_token.line, p->cur_token.col, show_token_type(t));
}

static Node
parse_expression(Parser* p, enum Precedence precedence) {
    PrefixParseFn *prefix = p->prefix_parse_fns[p->cur_token.type];
    if (prefix == NULL) {
        no_prefix_parse_error(p, p->cur_token.type);
        free(p->cur_token.literal);
        return (Node){};
    }
    Node left_exp = prefix(p);
    if (left_exp.obj == NULL)
        return (Node){};

    while (!peek_token_is(p, t_Semicolon)
            && precedence < peek_precedence(p)) {
        InfixParseFn *infix = p->infix_parse_fns[p->peek_token.type];
        if (infix == NULL)
            return left_exp;

        next_token(p);

        left_exp = infix(p, left_exp);
        if (left_exp.obj == NULL)
            return (Node){};
    };
    return left_exp;
}

static Node
parse_identifier(Parser* p) {
    Identifier* id = allocate(sizeof(Identifier));
    id->tok = p->cur_token;
    return NODE(n_Identifier, id);
}

static Node
parse_integer_literal(Parser* p) {
    IntegerLiteral* il = allocate(sizeof(IntegerLiteral));
    il->tok = p->cur_token;

    char* literal = p->cur_token.literal;
    char* endptr;
    int base = 0;

    if (literal[1] == '\0') {
        il->value = literal[0] - '0';
        return NODE(n_IntegerLiteral, il);
    }

    if ('b' == literal[1]) {
        base = 2; // skip '0b',
                  // [strtol] infers base 16 or 8 not base 2.
    }

    long value = strtol(p->cur_token.literal + base, &endptr, base);
    if (*endptr != '\0' || errno == ERANGE) {
        parser_error(p,
                ":%d,%d: could not parse '%s' as integer",
                p->cur_token.line, p->cur_token.col, literal);
        free(p->cur_token.literal);
        free(il);
        return (Node){};
    }

    il->value = value;
    return NODE(n_IntegerLiteral, il);
}

static Node
parse_float_literal(Parser* p) {
    FloatLiteral* fl = allocate(sizeof(FloatLiteral));
    fl->tok = p->cur_token;

    char* endptr;
    double value = strtod(fl->tok.literal, &endptr);
    if (*endptr != '\0' || errno == ERANGE) {
        parser_error(p,
                ":%d,%d: could not parse '%s' as float",
                fl->tok.line, fl->tok.col,
                fl->tok.literal);
        free(fl->tok.literal);
        free(fl);
        return (Node){};
    }

    fl->value = value;
    return NODE(n_FloatLiteral, fl);
}

static Node
parse_infix_expression(Parser* p, Node left) {
    InfixExpression* ie = allocate(sizeof(InfixExpression));
    ie->tok = p->cur_token;
    ie->op = p->cur_token.literal;
    ie->left = left;

    enum Precedence precedence = cur_precedence(p);
    next_token(p);
    ie->right = parse_expression(p, precedence);
    if (ie->right.obj == NULL) {
        node_free(left);
        free(ie->tok.literal);
        free(ie);
        return (Node){};
    }

    return NODE(n_InfixExpression, ie);
}

static Node
parse_prefix_expression(Parser* p) {
    PrefixExpression* pe = allocate(sizeof(PrefixExpression));
    pe->tok = p->cur_token;
    pe->op = p->cur_token.literal;

    next_token(p);

    pe->right = parse_expression(p, p_Prefix);
    if (pe->right.obj == NULL) {
        free(pe->tok.literal);
        free(pe);
        return (Node){};
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
    free(p->cur_token.literal); // '(' token
    next_token(p);
    Node n = parse_expression(p, p_Lowest);
    if (n.obj == NULL || !expect_peek(p, t_Rparen)) {
        node_free(n);
        return (Node){};
    }
    free(p->cur_token.literal); // ')' token
    return n;
}

static BlockStatement*
parse_block_statement(Parser* p) {
    BlockStatement* bs = allocate(sizeof(BlockStatement));
    bs->tok = p->cur_token;
    NodeBufferInit(&bs->stmts);

    next_token(p);

    while (!cur_token_is(p, t_Rbrace) && !cur_token_is(p, t_Eof)) {
        Node stmt = parse_statement(p);
        if (stmt.obj != NULL) {
            NodeBufferPush(&bs->stmts, stmt);
        }
        next_token(p);
    }

    if (!cur_token_is(p, t_Rbrace)) {
        cur_tok_error(p, t_Rbrace);
        free_block_statement(bs);
        return NULL;
    }

    free(p->cur_token.literal); // '}' tok
    return bs;
}

// returns -1 on err
static int
parse_function_parameters(Parser* p, FunctionLiteral* fl) {
    ParamBufferInit(&fl->params);

    free(p->cur_token.literal); // '(' tok
    next_token(p);

    if (cur_token_is(p, t_Rparen)) {
        free(p->cur_token.literal); // ')' tok
        return 0;
    }

    if (p->cur_token.type != t_Ident) {
        cur_tok_error(p, t_Ident);
        free(p->cur_token.literal);
        return -1;
    }

    Identifier* ident = allocate(sizeof(Identifier));
    ident->tok = p->cur_token;
    ident->tok.literal = p->cur_token.literal;
    ParamBufferPush(&fl->params, ident);

    while (peek_token_is(p, t_Comma)) {
        next_token(p);
        free(p->cur_token.literal); // ',' tok

        if (!expect_peek(p, t_Ident))
            return -1;

        ident = allocate(sizeof(Identifier));
        ident->tok = p->cur_token;
        ident->tok.literal = p->cur_token.literal;
        ParamBufferPush(&fl->params, ident);
    }

    if (!expect_peek(p, t_Rparen))
        return -1;

    free(p->cur_token.literal); // ')' tok

    return 0;
}

static Node
parse_function_literal(Parser* p) {
    FunctionLiteral* fl = allocate(sizeof(FunctionLiteral));
    fl->tok = p->cur_token;
    fl->name = NULL;

    if (!expect_peek(p, t_Lparen)) {
        free(fl->tok.literal);
        free(fl);
        return (Node){};
    }

    int err = parse_function_parameters(p, fl);
    if (err == -1 || !expect_peek(p, t_Lbrace)) {
        fl->body = NULL;
        free_function_literal(fl);
        return (Node){};
    }

    fl->body = parse_block_statement(p);
    if (fl->body == NULL) {
        free_function_literal(fl);
        return (Node){};
    }

    return NODE(n_FunctionLiteral, fl);
}

static Node
parse_string_literal(Parser* p) {
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
        free(al->tok.literal);
        free(al);
        return (Node){};
    }
    return NODE(n_ArrayLiteral, al);
}

static Node
parse_hash_literal(Parser* p) {
    HashLiteral* hl = allocate(sizeof(HashLiteral));
    hl->tok = p->cur_token;
    PairBufferInit(&hl->pairs);

    while (!peek_token_is(p, t_Rbrace)) {
        next_token(p);

        Node key = parse_expression(p, p_Lowest);
        if (key.obj == NULL) {
            free_hash_literal(hl);
            return (Node){};
        }

        if (!expect_peek(p, t_Colon)) {
            node_free(key);
            free_hash_literal(hl);
            return (Node){};
        }

        free(p->cur_token.literal); // ':' tok
        next_token(p);

        Node value = parse_expression(p, p_Lowest);
        if (value.obj == NULL) {
            free(p->cur_token.literal);
            node_free(key);
            free_hash_literal(hl);
            return (Node){};
        }

        PairBufferPush(&hl->pairs, (Pair){ key, value });

        if (peek_token_is(p, t_Rbrace)) break;

        if (!expect_peek(p, t_Comma)) {
            free_hash_literal(hl);
            return (Node){};
        }
        free(p->cur_token.literal);
    }

    if (!expect_peek(p, t_Rbrace)) {
        free_hash_literal(hl);
        return (Node){};
    }
    free(p->cur_token.literal);

    return NODE(n_HashLiteral, hl);
}

static Node
parse_call_expression(Parser* p, Node function) {
    CallExpression* ce = allocate(sizeof(CallExpression));
    ce->tok = p->cur_token;
    ce->function = function;
    ce->args = parse_expression_list(p, t_Rparen);
    if (ce->args.length == -1) {
        free(ce->tok.literal);
        node_free(function);
        free(ce);
        return (Node){};
    }
    return NODE(n_CallExpression, ce);
}

static Node
parse_index_expression(Parser* p, Node left) {
    IndexExpression* ie = allocate(sizeof(IndexExpression));
    ie->tok = p->cur_token;
    ie->left = left;
    next_token(p);
    ie->index = parse_expression(p, p_Lowest);
    if (ie->index.obj == NULL || !expect_peek(p, t_Rbracket)) {
        free(ie->tok.literal);
        node_free(left);
        free(ie);
        return (Node){};
    }
    free(p->cur_token.literal); // '}' tok
    return NODE(n_IndexExpression, ie);
}

static NodeBuffer
__parse_expression_list_err(NodeBuffer buf) {
    if (buf.data != NULL) {
        for (int i = 0; i < buf.length; i++)
            node_free(buf.data[i]);
        free(buf.data);
        buf.data = NULL;
    }
    buf.length = -1;
    return buf;
}

// [NodeBuffer.length] is -1 on err.
static NodeBuffer
parse_expression_list(Parser* p, TokenType end) {
    NodeBuffer list;
    NodeBufferInit(&list);

    if (peek_token_is(p, end)) {
        next_token(p);
        free(p->cur_token.literal);
        return list;
    }

    next_token(p);

    Node exp = parse_expression(p, p_Lowest);
    if (exp.obj == NULL)
        return __parse_expression_list_err(list);
    NodeBufferPush(&list, exp);

    while (peek_token_is(p, t_Comma)) {
        next_token(p);
        free(p->cur_token.literal); // ',' tok
        next_token(p);

        Node exp = parse_expression(p, p_Lowest);
        if (exp.obj == NULL)
            return __parse_expression_list_err(list);
        NodeBufferPush(&list, exp);
    }

    if (!expect_peek(p, end))
        return __parse_expression_list_err(list);

    free(p->cur_token.literal); // 'end' tok
    return list;
}

static Node
parse_if_expression(Parser* p) {
    IfExpression* ie = allocate(sizeof(IfExpression));
    ie->tok = p->cur_token;
    ie->consequence = NULL;
    ie->alternative = NULL;
    if (!expect_peek(p, t_Lparen)) {
        free(ie->tok.literal);
        free(ie);
        return (Node){};
    }
    free(p->cur_token.literal); // '(' tok

    next_token(p);
    ie->condition = parse_expression(p, p_Lowest);
    if (ie->condition.obj == NULL || !expect_peek(p, t_Rparen)) {
        free(ie->tok.literal);
        free(ie);
        return (Node){};
    }
    free(p->cur_token.literal); // ')' tok

    if (!expect_peek(p, t_Lbrace)) {
        node_free(NODE(n_IfExpression, ie));
        return (Node){};
    }

    ie->consequence = parse_block_statement(p);
    if (ie->consequence == NULL) {
        node_free(NODE(n_IfExpression, ie));
        return (Node){};
    }

    if (peek_token_is(p, t_Else)) {
        next_token(p);
        free(p->cur_token.literal); // 'else' tok

        if (!expect_peek(p, t_Lbrace)) {
            node_free(NODE(n_IfExpression, ie));
            return (Node){};
        }

        ie->alternative = parse_block_statement(p);
        if (ie->alternative == NULL) {
            node_free(NODE(n_IfExpression, ie));
            return (Node){};
        }
    }

    return NODE(n_IfExpression, ie);
}

static Node
parse_let_statement(Parser* p) {
    LetStatement* stmt = allocate(sizeof(LetStatement));
    stmt->tok = p->cur_token;

    if (!expect_peek(p, t_Ident)) {
        free(stmt->tok.literal);
        free(stmt);
        return (Node){};
    }

    stmt->name = allocate(sizeof(Identifier));
    stmt->name->tok = p->cur_token;

    if (!expect_peek(p, t_Assign)) {
        free(stmt->tok.literal);
        free(stmt->name->tok.literal);
        free(stmt->name);
        free(stmt);
        return (Node){};
    }
    free(p->cur_token.literal); // '=' tok
    next_token(p);

    stmt->value = parse_expression(p, p_Lowest);
    if (stmt->value.obj == NULL) {
        node_free(NODE(n_LetStatement, stmt));
        return (Node){};
    }

    if (stmt->value.typ == n_FunctionLiteral) {
        FunctionLiteral *fl = stmt->value.obj;
        fl->name = stmt->name->tok.literal;
    }

    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
        free(p->cur_token.literal);
    }
    return NODE(n_LetStatement, stmt);
}

static Node
parse_return_statement(Parser* p) {
    ReturnStatement* stmt = allocate(sizeof(ReturnStatement));
    stmt->tok = p->cur_token;

    next_token(p);

    stmt->return_value = parse_expression(p, p_Lowest);
    if (stmt->return_value.obj == NULL) {
        free(stmt->tok.literal);
        free(stmt);
        return (Node){};
    }

    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
        free(p->cur_token.literal);
    }
    return NODE(n_ReturnStatement, stmt);
}

static Node
parse_expression_statement(Parser* p) {
    ExpressionStatement* stmt = allocate(sizeof(ExpressionStatement));
    stmt->tok = p->cur_token; // will be the same as stmt.expression.tok

    stmt->expression = parse_expression(p, p_Lowest);
    if (stmt->expression.obj == NULL) {
        free(stmt);
        return (Node){};
    }

    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
        free(p->cur_token.literal);
    }
    return NODE(n_ExpressionStatement, stmt);
}

// on failure, return Node with `n.obj` == NULL
static Node
parse_statement(Parser* p) {
    switch (p->cur_token.type) {
    case t_Let:
        return parse_let_statement(p);
    case t_Return:
        return parse_return_statement(p);
    case t_Illegal:
        parser_error(p,
                ":%d,%d: illegal character '%s'",
                p->cur_token.line, p->cur_token.col, p->cur_token.literal);
        free(p->cur_token.literal);
        return (Node){};
    default:
        return parse_expression_statement(p);
    }
}

void parser_init(Parser* p) {
    memset(p, 0, sizeof(Parser));

    p->prefix_parse_fns[t_Ident] = parse_identifier;
    p->prefix_parse_fns[t_Int] = parse_integer_literal;
    p->prefix_parse_fns[t_Float] = parse_float_literal;
    p->prefix_parse_fns[t_Bang] = parse_prefix_expression;
    p->prefix_parse_fns[t_Minus] = parse_prefix_expression;
    p->prefix_parse_fns[t_True] = parse_boolean;
    p->prefix_parse_fns[t_False] = parse_boolean;
    p->prefix_parse_fns[t_Lparen] = parse_grouped_expression;
    p->prefix_parse_fns[t_If] = parse_if_expression;
    p->prefix_parse_fns[t_Function] = parse_function_literal;
    p->prefix_parse_fns[t_String] = parse_string_literal;
    p->prefix_parse_fns[t_Lbracket] = parse_array_literal;
    p->prefix_parse_fns[t_Lbrace] = parse_hash_literal;

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
        free(p->errors.data[i]);
    }
    free(p->errors.data);
    free(p->peek_token.literal);
}

Program parse(Parser* p, const char *input) {
    Lexer l;
    lexer_init(&l, input);
    p->l = &l;
    p->cur_token = lexer_next_token(&l);
    p->peek_token = lexer_next_token(&l);

    Program prog = {};
    while (p->cur_token.type != t_Eof) {
        Node stmt = parse_statement(p);
        if (stmt.obj != NULL) {
            NodeBufferPush(&prog.stmts, stmt);
        } else if (p->errors.length >= MAX_ERRORS) {
            parser_error(p, "too many errors, stopping now\n");
            break;
        }

        next_token(p);
    }
    return prog;
}

void program_free(Program* p) {
    for (int i = 0; i < p->stmts.length; i++) {
        node_free(p->stmts.data[i]);
    }
    free(p->stmts.data);
}
