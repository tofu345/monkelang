#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>

#define START_CAP 8

static void exit_nomem() {
    fprintf(stderr, "no memory");
    exit(1);
}

static void next_token(Parser* p) {
    p->cur_token = p->peek_token;
    p->peek_token = lexer_next_token(p->l);
}

Parser parser_new(Lexer* l) {
    Parser p = {};
    p.l = l;
    p.errors = malloc(START_CAP * sizeof(char*));
    p.errors_cap = START_CAP;
    p.errors_len = 0;

    // Read two tokens, so curToken and peekToken are both set
    next_token(&p);
    next_token(&p);
    return p;
}

static void peek_error(Parser* p, TokenType t) {
    char* msg = NULL;
    int len = asprintf(&msg,
            "expected next token to be '%s', got '%s' instead",
            show_token_type(t), show_token_type(p->peek_token.type));
    if (len == -1) exit_nomem();

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

void parser_destroy(Parser* p) {
    for (size_t i = 0; i < p->errors_len; i++) {
        free(p->errors[i]);
    }
    free(p->errors);
}

static bool cur_token_is(Parser* p, TokenType t) {
    return p->cur_token.type == t;
}

static bool peek_token_is(Parser* p, TokenType t) {
    return p->peek_token.type == t;
}

static bool expect_peek(Parser* p, TokenType t) {
    if (peek_token_is(p, t)) {
        next_token(p);
        return true;
    } else {
        peek_error(p, t);
        return false;
    }
}

static Node parse_let_statement(Parser* p) {
    LetStatement* stmt = malloc(sizeof(LetStatement));
    if (stmt == NULL) exit_nomem();
    stmt->tok = p->cur_token;
    stmt->value = (Node){};
    Node n = { n_LetStatement, stmt };

    if (!expect_peek(p, t_Ident)) {
        node_destroy(&n);
        return (Node){};
    }

    stmt->name = malloc(sizeof(Identifier));
    if (stmt->name == NULL) exit_nomem();
    stmt->name->tok = p->cur_token;
    stmt->name->value = p->cur_token.literal;

    if (!expect_peek(p, t_Assign)) {
        node_destroy(&n);
        return (Node){};
    }

    // TODO: the boss says we are skipping expressions until we
    // encounter a semicolon
    while (!cur_token_is(p, t_Semicolon)) {
        next_token(p);
        free(p->cur_token.literal);
    }

    return n;
}

// on failure returns a Node with `n.typ` and `n.obj` == 0
static Node parse_statement(Parser* p) {
    switch (p->cur_token.type) {
    case t_Let:
        return parse_let_statement(p);
    default:
        return (Node){};
    }
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
