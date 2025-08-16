#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

bool is_expression(Node* n) {
    return n->typ < n_LetStatement;
}

bool is_statement(Node* n) {
    return n->typ >= n_LetStatement;
}

char* token_literal(Node* n) {
    // since the first element of every node is a token.
    Token* tok = (Token*)n->obj;
    return tok->literal;
}

void node_destroy(Node* n) {
    if (n->obj == NULL) return;

    // since the first element of every node is a token.
    // oh i can't wait for the segfaults because of this
    Token* tok = (Token*)n->obj;
    free(tok->literal);

    switch (n->typ) {
    case n_LetStatement:
        LetStatement* l_stmt = (LetStatement*)n->obj;
        node_destroy(&l_stmt->value);
        if (l_stmt->name != NULL)
            free(l_stmt->name->value);
        free(l_stmt);
        break;
    default:
        free(n->obj);
        break;
    }
}

char* program_token_literal(Program* p) {
    if (p->len > 0) {
        return token_literal(&p->statements[0]);
    } else {
        return "";
    }
}

void program_destroy(Program* p) {
    for (size_t i = 0; i < p->len; i++) {
        node_destroy(&p->statements[i]);
    }
}
