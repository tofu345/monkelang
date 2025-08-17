#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// macro programming, wow
#define FPRINTF(fp, ...) \
    if (fprintf(fp, __VA_ARGS__) <= 0) \
        return -1;

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

int node_fprint(const Node n, FILE* fp) {
    if (n.obj == NULL) return 0;

    // i despise this indentation
    switch (n.typ) {
        case n_Identifier:
            {
                Identifier* i = n.obj;
                FPRINTF(fp, "%s", i->value);
                return 0;
            }

        case n_IntegerLiteral:
            {
                IntegerLiteral* il = n.obj;
                FPRINTF(fp, "%ld", il->value);
                return 0;
            }

        case n_PrefixExpression:
            {
                PrefixExpression* pe = n.obj;
                FPRINTF(fp, "(%s", pe->op);
                node_fprint(pe->right, fp);
                FPRINTF(fp, ")");
                return 0;
            }

        case n_InfixExpression:
            {
                InfixExpression* ie = n.obj;
                FPRINTF(fp, "(");
                node_fprint(ie->left, fp);
                FPRINTF(fp, " %s ", ie->op);
                node_fprint(ie->right, fp);
                FPRINTF(fp, ")");
                return 0;
            }

        case n_LetStatement:
            {
                LetStatement* ls = n.obj;
                FPRINTF(fp, "%s %s = ",
                        ls->tok.literal, ls->name->value);
                if (ls->value.obj != NULL)
                    node_fprint(ls->value, fp);
                FPRINTF(fp, ";");
                return 0;
            }

        case n_ReturnStatement:
            {
                ReturnStatement* rs = n.obj;
                FPRINTF(fp, "%s ", rs->tok.literal);
                if (rs->return_value.obj != NULL)
                    node_fprint(rs->return_value, fp);
                FPRINTF(fp, ";");
                return 0;
            }

        case n_ExpressionStatement:
            {
                ExpressionStatement* es = n.obj;
                if (n.obj != NULL)
                    node_fprint(es->expression, fp);
                return 0;
            }

        default:
            {
                return -1;
            }
    }
}

void node_destroy(Node n) {
    if (n.obj == NULL) return;

    switch (n.typ) {
        case n_PrefixExpression:
            {
                PrefixExpression* pe = n.obj;
                free(pe->tok.literal);
                node_destroy(pe->right);
                free(pe);
                break;
            }

        case n_InfixExpression:
            {
                InfixExpression* ie = n.obj;
                free(ie->tok.literal);
                node_destroy(ie->left);
                node_destroy(ie->right);
                free(ie);
                break;
            }

        case n_LetStatement:
            {
                LetStatement* ls = n.obj;
                free(ls->tok.literal);
                node_destroy(ls->value);
                if (ls->name != NULL)
                    free(ls->name->value);
                free(ls);
                break;
            }

        case n_ReturnStatement:
            {
                ReturnStatement* rs = n.obj;
                free(rs->tok.literal);
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
            {
                // since first field of `n.obj` is `Token`
                Token* tok = n.obj;
                free(tok->literal);
                free(n.obj);
                break;
            }
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

int program_fprint(const Program* p, FILE* fp) {
    for (size_t i = 0; i < p->len; i++) {
        if (node_fprint(p->statements[i], fp) == -1)
            return -1;
    }
    return 0;
}
