#include "ast.h"
#include "grow_array.h"
#include "lexer.h"
#include "parser.h"
#include "parser_tracing.h"
#include "token.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Node parse_statement(Parser* p);

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
exit_nomem() {
    fprintf(stderr, "no memory");
    exit(1);
}

static void
parser_error(Parser* p, char* msg) {
    if (p->errors_len >= p->errors_cap)
        grow_array((void**)&p->errors, &p->errors_cap, sizeof(char*));
    p->errors[p->errors_len] = msg;
    p->errors_len++;
}

static void
next_token(Parser* p) {
    p->cur_token = p->peek_token;
    p->peek_token = lexer_next_token(p->l);
}

static void
peek_error(Parser* p, TokenType t) {
    char* msg = NULL;
    int len = asprintf(&msg,
            "expected next token to be '%s', got '%s' instead",
            show_token_type(t), show_token_type(p->peek_token.type));
    if (len == -1) exit_nomem();
    parser_error(p, msg);
}

static bool
cur_token_is(const Parser* p, TokenType t) {
    return p->cur_token.type == t;
}

static bool
peek_token_is(const Parser* p, TokenType t) {
    return p->peek_token.type == t;
}

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
    char* msg = NULL;
    int err = asprintf(&msg, "no prefix parse function for '%s' found", show_token_type(t));
    if (err == -1) exit_nomem();
    parser_error(p, msg);
}

static Node
parse_expression(Parser* p, enum Precedence precedence) {
    trace("parse_expression");
    PrefixParseFn prefix = p->prefix_parse_fns[p->cur_token.type];
    if (prefix == NULL) {
        no_prefix_parse_error(p, p->cur_token.type);
        return (Node){};
    }
    Node left_exp = prefix(p);
    if (left_exp.obj == NULL)
        return (Node){};

    while (!peek_token_is(p, t_Semicolon)
            && precedence < peek_precedence(p)) {
        InfixParseFn infix = p->infix_parse_fns[p->peek_token.type];
        if (infix == NULL)
            return left_exp;

        next_token(p);

        left_exp = infix(p, left_exp);
        if (left_exp.obj == NULL)
            return (Node){};
    };
    untrace("parse_expression");
    return left_exp;
}

static Node
parse_identifier(Parser* p) {
    Identifier* id = malloc(sizeof(Identifier));
    if (id == NULL) exit_nomem();
    id->tok = p->cur_token;
    id->value = p->cur_token.literal;
    return (Node){ n_Identifier, id };
}

static Node
parse_integer_literal(Parser* p) {
    trace("parse_integer_literal");
    IntegerLiteral* int_lit = malloc(sizeof(IntegerLiteral));
    if (int_lit == NULL) exit_nomem();
    int_lit->tok = p->cur_token;

    // TODO: base 2, 8, 16
    long value = strtol(p->cur_token.literal, NULL, 10);
    if (errno == ERANGE) {
        char* msg = NULL;
        int err = asprintf(&msg, "could not parse '%s' as integer", p->cur_token.literal);
        if (err == -1) exit_nomem();
        parser_error(p, msg);
        free(p->cur_token.literal);
        free(int_lit);
        return (Node){};
    }

    int_lit->value = value;
    untrace("parse_integer_literal");
    return (Node){ n_IntegerLiteral, int_lit };
}

static Node
parse_infix_expression(Parser* p, Node left) {
    trace("parse_infix_expression");
    InfixExpression* ie = malloc(sizeof(InfixExpression));
    ie->tok = p->cur_token;
    ie->op = p->cur_token.literal;
    ie->left = left;

    enum Precedence precedence = cur_precedence(p);
    next_token(p);
    ie->right = parse_expression(p, precedence);
    if (ie->right.obj == NULL) {
        node_destroy(left);
        free(ie->tok.literal);
        free(ie);
        return (Node){};
    }

    untrace("parse_infix_expression");
    return (Node){ n_InfixExpression, ie };
}

static Node
parse_prefix_expression(Parser* p) {
    trace("parse_prefix_expression");
    PrefixExpression* pe = malloc(sizeof(PrefixExpression));
    pe->tok = p->cur_token;
    pe->op = p->cur_token.literal;

    next_token(p);

    pe->right = parse_expression(p, p_Prefix);
    if (pe->right.obj == NULL) {
        free(pe->tok.literal);
        free(pe);
        return (Node){};
    }

    untrace("parse_prefix_expression");
    return (Node){ n_PrefixExpression, pe };
}

static Node
parse_boolean(Parser* p) {
    Boolean* b = malloc(sizeof(Boolean));
    b->tok = p->cur_token;
    b->value = cur_token_is(p, t_True);
    return (Node){ n_Boolean, b };
}

static Node
parse_grouped_expression(Parser* p) {
    free(p->cur_token.literal); // '(' token
    next_token(p);
    Node n = parse_expression(p, p_Lowest);
    if (!expect_peek(p, t_Rparen)) {
        node_destroy(n);
        return (Node){};
    }
    free(p->cur_token.literal); // ')' token
    return n;
}

static BlockStatement*
parse_block_statement(Parser* p) {
    BlockStatement* bs = malloc(sizeof(BlockStatement));
    bs->tok = p->cur_token;
    bs->statements = malloc(START_CAPACITY * sizeof(Node));
    if (bs->statements == NULL) exit_nomem();
    bs->cap = START_CAPACITY;
    bs->len = 0;

    next_token(p);

    while (!cur_token_is(p, t_Rbrace) && !cur_token_is(p, t_Eof)) {
        Node stmt = parse_statement(p);
        if (stmt.obj != NULL) {
            if (bs->len >= bs->cap)
                grow_array((void**)&bs->statements, &bs->cap, sizeof(Node));
            bs->statements[bs->len] = stmt;
            bs->len++;
        }
        next_token(p);
    }
    free(p->cur_token.literal); // '}' token

    return bs;
}

static int
parse_function_parameters(Parser* p, FunctionLiteral* fl) {
    fl->params = malloc(START_CAPACITY * sizeof(Identifier*));
    fl->params_len = 0;
    fl->params_cap = START_CAPACITY;

    if (peek_token_is(p, t_Rparen)) {
        free(p->cur_token.literal); // '(' tok
        next_token(p);
        free(p->cur_token.literal); // ')' tok
        return 0;
    }

    free(p->cur_token.literal); // '(' tok
    next_token(p);

    if (p->cur_token.type != t_Ident) {
        char* msg = NULL;
        int len = asprintf(&msg,
                "expected first function parameter to be '%s', got '%s' instead",
                show_token_type(t_Ident), show_token_type(p->cur_token.type));
        if (len == -1) exit_nomem();
        parser_error(p, msg);
        free(p->cur_token.literal);
        return -1;
    }

    Identifier* ident = malloc(sizeof(Identifier));
    ident->tok = p->cur_token;
    ident->value = p->cur_token.literal;
    fl->params[0] = ident;
    fl->params_len = 1;

    while (peek_token_is(p, t_Comma)) {
        next_token(p);
        free(p->cur_token.literal); // ',' tok
        if (!expect_peek(p, t_Ident))
            return -1;

        ident = malloc(sizeof(Identifier));
        ident->tok = p->cur_token;
        ident->value = p->cur_token.literal;

        if (fl->params_len >= fl->params_cap)
            grow_array((void**)&fl->params, &fl->params_cap,
                    sizeof(Identifier*));
        fl->params[fl->params_len] = ident;
        fl->params_len++;
    }

    if (!expect_peek(p, t_Rparen))
        return -1;

    free(p->cur_token.literal); // ')' tok

    return 0;
}

static Node
parse_function_literal(Parser* p) {
    FunctionLiteral* fl = malloc(sizeof(FunctionLiteral));
    fl->tok = p->cur_token;
    if (!expect_peek(p, t_Lparen)) {
        free(fl->tok.literal);
        free(fl);
        return (Node){};
    }

    if (parse_function_parameters(p, fl) == -1
            || !expect_peek(p, t_Lbrace)) {
        for (size_t i = 0; i < fl->params_len; i++) {
            free(fl->params[i]->tok.literal);
            free(fl->params[i]);
        }
        free(fl->params);
        free(fl->tok.literal);
        free(fl);
        return (Node){};
    }

    fl->body = parse_block_statement(p);
    return (Node){ n_FunctionLiteral, fl };
}

static int
parse_call_arguments(Parser* p, CallExpression* ce) {
    ce->args = malloc(START_CAPACITY * sizeof(Node));
    ce->args_len = 0;
    ce->args_cap = START_CAPACITY;

    if (peek_token_is(p, t_Rparen)) {
        next_token(p);
        free(p->cur_token.literal); // ')' tok
        return 0;
    }

    next_token(p);

    ce->args[0] = parse_expression(p, p_Lowest);
    ce->args_len = 1;

    while (peek_token_is(p, t_Comma)) {
        next_token(p);
        free(p->cur_token.literal); // ',' tok
        next_token(p);

        Node exp = parse_expression(p, p_Lowest);
        if (exp.obj == NULL) {
            return -1;
        }

        if (ce->args_len >= ce->args_cap)
            grow_array((void**)&ce->args, &ce->args_cap, sizeof(Node));
        ce->args[ce->args_len] = exp;
        ce->args_len++;
    }

    if (!expect_peek(p, t_Rparen))
        return -1;

    return 0;
}

static Node
parse_call_expression(Parser* p, Node function) {
    CallExpression* ce = malloc(sizeof(CallExpression));
    ce->tok = p->cur_token;
    ce->function = function;
    if (parse_call_arguments(p, ce) == -1) {
        for (size_t i = 0; i < ce->args_len; i++) {
            node_destroy(ce->args[i]);
        }
        free(ce->args);
        free(ce->tok.literal);
        node_destroy(function);
        free(ce);
        return (Node){};
    }
    free(p->cur_token.literal);
    return (Node){ n_CallExpression, ce };
}

static Node
parse_if_expression(Parser* p) {
    IfExpression* ie = malloc(sizeof(IfExpression));
    ie->tok = p->cur_token;
    ie->alternative = NULL;
    if (!expect_peek(p, t_Lparen)) {
        free(ie->tok.literal);
        free(ie);
        return (Node){};
    }
    free(p->cur_token.literal); // '(' tok

    next_token(p);
    ie->condition = parse_expression(p, p_Lowest);
    if (!expect_peek(p, t_Rparen)) {
        node_destroy(ie->condition);
        free(ie->tok.literal);
        free(ie);
        return (Node){};
    }
    free(p->cur_token.literal); // ')' tok

    if (!expect_peek(p, t_Lbrace)) {
        node_destroy(ie->condition);
        free(ie->tok.literal);
        free(ie);
        return (Node){};
    }

    ie->consequence = parse_block_statement(p);

    if (peek_token_is(p, t_Else)) {
        next_token(p);
        free(p->cur_token.literal); // 'else' tok
        if (!expect_peek(p, t_Lbrace)) {
            node_destroy(ie->condition);
            node_destroy((Node){ n_BlockStatement, ie->consequence });
            free(ie->tok.literal);
            free(ie);
            return (Node){};
        }
        ie->alternative = parse_block_statement(p);
    }

    return (Node){ n_IfExpression, ie };
}

static Node
parse_let_statement(Parser* p) {
    LetStatement* stmt = malloc(sizeof(LetStatement));
    if (stmt == NULL) exit_nomem();
    stmt->tok = p->cur_token;

    if (!expect_peek(p, t_Ident)) {
        free(stmt->tok.literal);
        free(stmt);
        return (Node){};
    }

    stmt->name = malloc(sizeof(Identifier));
    if (stmt->name == NULL) exit_nomem();
    stmt->name->tok = p->cur_token;
    stmt->name->value = p->cur_token.literal;

    if (!expect_peek(p, t_Assign)) {
        free(stmt->name->tok.literal);
        free(stmt->name);
        free(stmt->tok.literal);
        free(stmt);
        return (Node){};
    }
    free(p->cur_token.literal); // '=' tok

    next_token(p);

    stmt->value = parse_expression(p, p_Lowest);

    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
        free(p->cur_token.literal);
    }

    return (Node){ n_LetStatement, stmt };
}

static Node
parse_return_statement(Parser* p) {
    ReturnStatement* stmt = malloc(sizeof(ReturnStatement));
    if (stmt == NULL) exit_nomem();
    stmt->tok = p->cur_token;

    next_token(p);

    stmt->return_value = parse_expression(p, p_Lowest);

    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
        free(p->cur_token.literal);
    }

    return (Node){ n_ReturnStatement, stmt };
}

static Node
parse_expression_statement(Parser* p) {
    trace("parse_expression_statement");
    ExpressionStatement* stmt = malloc(sizeof(ExpressionStatement));
    if (stmt == NULL) exit_nomem();
    stmt->tok = p->cur_token;
    stmt->expression = parse_expression(p, p_Lowest);
    if (stmt->expression.obj == NULL) {
        free(stmt);
        return (Node){};
    }

    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
        free(p->cur_token.literal);
    }

    untrace("parse_expression_statement");
    return (Node){ n_ExpressionStatement, stmt };
}

// on failure, return Node with `n.obj` == NULL
static Node
parse_statement(Parser* p) {
    switch (p->cur_token.type) {
    case t_Let:
        return parse_let_statement(p);
    case t_Return:
        return parse_return_statement(p);
    default:
        return parse_expression_statement(p);
    }
}

Parser* parser_new(Lexer* l) {
    Parser* p = calloc(1, sizeof(Parser));
    p->l = l;
    p->errors = malloc(START_CAPACITY * sizeof(char*));
    p->errors_cap = START_CAPACITY;

    p->prefix_parse_fns[t_Ident] = parse_identifier;
    p->prefix_parse_fns[t_Int] = parse_integer_literal;
    p->prefix_parse_fns[t_Bang] = parse_prefix_expression;
    p->prefix_parse_fns[t_Minus] = parse_prefix_expression;
    p->prefix_parse_fns[t_True] = parse_boolean;
    p->prefix_parse_fns[t_False] = parse_boolean;
    p->prefix_parse_fns[t_Lparen] = parse_grouped_expression;
    p->prefix_parse_fns[t_If] = parse_if_expression;
    p->prefix_parse_fns[t_Function] = parse_function_literal;

    p->infix_parse_fns[t_Plus] = parse_infix_expression;
    p->infix_parse_fns[t_Minus] = parse_infix_expression;
    p->infix_parse_fns[t_Slash] = parse_infix_expression;
    p->infix_parse_fns[t_Asterisk] = parse_infix_expression;
    p->infix_parse_fns[t_Eq] = parse_infix_expression;
    p->infix_parse_fns[t_Not_eq] = parse_infix_expression;
    p->infix_parse_fns[t_Lt] = parse_infix_expression;
    p->infix_parse_fns[t_Gt] = parse_infix_expression;
    p->infix_parse_fns[t_Lparen] = parse_call_expression;

    // Read two tokens, so curToken and peekToken are both set
    p->cur_token = lexer_next_token(p->l);
    p->peek_token = lexer_next_token(p->l);
    return p;
}

void parser_destroy(Parser* p) {
    for (size_t i = 0; i < p->errors_len; i++) {
        free(p->errors[i]);
    }
    free(p->errors);
    free(p->peek_token.literal);
    free(p);
}

Program parse_program(Parser* p) {
    Program prog;
    prog.stmts = malloc(START_CAPACITY * sizeof(Node));
    if (prog.stmts == NULL) exit_nomem();
    prog.cap = START_CAPACITY;
    prog.len = 0;

    while (p->cur_token.type != t_Eof) {
        Node stmt = parse_statement(p);
        if (stmt.obj == NULL)
            break;

        if (prog.len >= prog.cap)
            grow_array((void**)&prog.stmts, &prog.cap, sizeof(Node));

        prog.stmts[prog.len] = stmt;
        prog.len++;

        next_token(p);
    }

    return prog;
}

void program_destroy(Program* p) {
    for (size_t i = 0; i < p->len; i++) {
        node_destroy(p->stmts[i]);
    }
    free(p->stmts);
}
