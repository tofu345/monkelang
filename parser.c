#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "parser_tracing.h"
#include "token.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define START_CAP 8

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
    if (p->errors_len >= p->errors_cap) {
        size_t new_cap = p->errors_cap * 2;
        char** new_errors = realloc(p->errors, new_cap * sizeof(char*));
        if (new_errors == NULL) exit_nomem();
        p->errors_cap = new_cap;
        p->errors = new_errors;
    }
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

    while (!peek_token_is(p, t_Semicolon)
            && precedence < peek_precedence(p)) {
        InfixParseFn infix = p->infix_parse_fns[p->peek_token.type];
        if (infix == NULL)
            return left_exp;

        next_token(p);
        left_exp = infix(p, left_exp);
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

    long value = strtol(p->cur_token.literal, NULL, 10);
    if (errno == ERANGE) {
        char* msg = NULL;
        int err = asprintf(&msg, "could not parse '%s' as integer", p->cur_token.literal);
        if (err == -1) exit_nomem();
        parser_error(p, msg);
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
    token_destroy(&p->cur_token); // free '(' token
    next_token(p);
    Node n = parse_expression(p, p_Lowest);
    if (!expect_peek(p, t_Rparen)) {
        return (Node){};
    }
    token_destroy(&p->cur_token); // free ')' token
    return n;
}

static Node
parse_let_statement(Parser* p) {
    LetStatement* stmt = malloc(sizeof(LetStatement));
    if (stmt == NULL) exit_nomem();
    stmt->tok = p->cur_token;

    if (!expect_peek(p, t_Ident)) {
        token_destroy(&stmt->tok);
        free(stmt);
        return (Node){};
    }

    stmt->name = malloc(sizeof(Identifier));
    if (stmt->name == NULL) exit_nomem();
    stmt->name->tok = p->cur_token;
    stmt->name->value = p->cur_token.literal;

    if (!expect_peek(p, t_Assign)) {
        token_destroy(&stmt->tok);
        token_destroy(&stmt->name->tok);
        free(stmt->name);
        free(stmt);
        return (Node){};
    }
    token_destroy(&p->cur_token);

    stmt->value = (Node){};
    // TODO: the boss says we are skipping expressions until we
    // encounter a semicolon
    while (!cur_token_is(p, t_Semicolon)) {
        next_token(p);
        token_destroy(&p->cur_token);
    }

    return (Node){ n_LetStatement, stmt };
}

static Node
parse_return_statement(Parser* p) {
    ReturnStatement* stmt = malloc(sizeof(ReturnStatement));
    if (stmt == NULL) exit_nomem();
    stmt->tok = p->cur_token;

    next_token(p);

    stmt->return_value = (Node){};
    // TODO: skipping the expressions until we encounter a semicolon
    while (!cur_token_is(p, t_Semicolon)) {
        token_destroy(&p->cur_token);
        next_token(p);
    }
    token_destroy(&p->cur_token);

    return (Node){ n_ReturnStatement, stmt };
}

static Node
parse_expression_statement(Parser* p) {
    trace("parse_expression_statement");
    ExpressionStatement* stmt = malloc(sizeof(ExpressionStatement));
    if (stmt == NULL) exit_nomem();
    stmt->tok = p->cur_token;
    stmt->expression = parse_expression(p, p_Lowest);

    if (peek_token_is(p, t_Semicolon)) {
        next_token(p);
        token_destroy(&p->cur_token);
    }

    untrace("parse_expression_statement");
    return (Node){ n_ExpressionStatement, stmt };
}

// on failure returns a Node with `n.typ` and `n.obj` == 0
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
    p->errors = malloc(START_CAP * sizeof(char*));
    p->errors_cap = START_CAP;

    p->prefix_parse_fns[t_Ident] = parse_identifier;
    p->prefix_parse_fns[t_Int] = parse_integer_literal;
    p->prefix_parse_fns[t_Bang] = parse_prefix_expression;
    p->prefix_parse_fns[t_Minus] = parse_prefix_expression;
    p->prefix_parse_fns[t_True] = parse_boolean;
    p->prefix_parse_fns[t_False] = parse_boolean;
    p->prefix_parse_fns[t_Lparen] = parse_grouped_expression;

    p->infix_parse_fns[t_Plus] = parse_infix_expression;
    p->infix_parse_fns[t_Minus] = parse_infix_expression;
    p->infix_parse_fns[t_Slash] = parse_infix_expression;
    p->infix_parse_fns[t_Asterisk] = parse_infix_expression;
    p->infix_parse_fns[t_Eq] = parse_infix_expression;
    p->infix_parse_fns[t_Not_eq] = parse_infix_expression;
    p->infix_parse_fns[t_Lt] = parse_infix_expression;
    p->infix_parse_fns[t_Gt] = parse_infix_expression;

    // Read two tokens, so curToken and peekToken are both set
    next_token(p);
    next_token(p);
    return p;
}

void parser_destroy(Parser* p) {
    for (size_t i = 0; i < p->errors_len; i++) {
        free(p->errors[i]);
    }
    free(p->errors);
    token_destroy(&p->peek_token);
    free(p);
}

Program parse_program(Parser* p) {
    Program prog;
    prog.statements = malloc(START_CAP * sizeof(Node));
    if (prog.statements == NULL) exit_nomem();
    prog.cap = START_CAP;
    prog.len = 0;

    while (p->cur_token.type != t_Eof) {
        Node stmt = parse_statement(p);
        if (stmt.obj != NULL) {
            if (prog.len >= prog.cap) {
                size_t new_cap = prog.cap * 2;
                Node* new_stmts =
                    realloc(prog.statements, new_cap * sizeof(Node));
                if (new_stmts == NULL) exit_nomem();
                prog.cap = new_cap;
                prog.statements = new_stmts;
            }

            prog.statements[prog.len] = stmt;
            prog.len++;
        }
        next_token(p);
    }

    return prog;
}

void program_destroy(Program* p) {
    for (size_t i = 0; i < p->len; i++) {
        node_destroy(p->statements[i]);
    }
    free(p->statements);
}

