#include "ast.h"

#include <stdlib.h>
#include <string.h>

bool is_expression(const Node n) {
    return n.typ < n_LetStatement;
}

bool is_statement(const Node n) {
    return n.typ >= n_LetStatement;
}

char* token_literal(const Node n) {
    // since the first element of every node is a token.
    Token* tok = n.obj;
    return tok->literal;
}

void node_print(const Node n, FILE* fp) {
    if (n.obj == NULL) return;

    // i despise this indentation
    switch (n.typ) {
        case n_Identifier:
            {
                Identifier* i = n.obj;
                fprintf(fp, "%s", i->value);
                break;
            }

        case n_LetStatement:
            {
                LetStatement* ls = n.obj;
                fprintf(fp, "%s %s = ",
                        ls->tok.literal, ls->name->value);
                if (ls->value.obj != NULL)
                    node_print(ls->value, fp);
                fprintf(fp, ";");
                break;
            }

        case n_ReturnStatement:
            {
                ReturnStatement* rs = n.obj;
                fprintf(fp, "%s ", rs->tok.literal);
                if (rs->return_value.obj != NULL)
                    node_print(rs->return_value, fp);
                fprintf(fp, ";");
                break;
            }

        case n_ExpressionStatement:
            {
                ExpressionStatement* es = n.obj;
                if (n.obj != NULL)
                    node_print(es->expression, fp);
                break;
            }
    }
}

void node_destroy(Node n) {
    if (n.obj == NULL) return;

    // since the first element of every node is a token.
    Token* tok = n.obj;
    // oh i can't wait for the segfaults because of this
    free(tok->literal);

    switch (n.typ) {
        case n_LetStatement:
            {
                LetStatement* ls = n.obj;
                node_destroy(ls->value);
                if (ls->name != NULL)
                    free(ls->name->value);
                free(ls);
                break;
            }

        case n_ReturnStatement:
            {
                ReturnStatement* rs = n.obj;
                node_destroy(rs->return_value);
                free(rs);
                break;
            }

        case n_ExpressionStatement:
            {
                ExpressionStatement* es = n.obj;
                node_destroy(es->expression);
                free(es);
                break;
            }

        default:
            free(n.obj);
            break;
    }
}

char* program_token_literal(const Program* p) {
    if (p->len > 0) {
        return token_literal(p->statements[0]);
    } else {
        return "";
    }
}

void program_destroy(Program* p) {
    for (size_t i = 0; i < p->len; i++) {
        node_destroy(p->statements[i]);
    }
}

void program_print(const Program* p, FILE* fp) {
    for (size_t i = 0; i < p->len; i++) {
        node_print(p->statements[i], fp);
    }
}
