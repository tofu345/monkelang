#include "ast.h"
#include "buffer.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// macro programming, wow
#define NODE_FPRINT(fp, ...) \
    if (node_fprint( __VA_ARGS__, fp) == -1) \
        return -1;

char* token_literal(const Node n) {
    // since the first element of every `node.obj` is a token.
    Token* tok = n.obj;
    return tok->literal;
}

static int
fprint_identifier(Identifier* i, FILE* fp) {
    FPRINTF(fp, "%s", i->value);
    return 0;
}

static int
fprint_integer_literal(IntegerLiteral* il, FILE* fp) {
    FPRINTF(fp, "%ld", il->value);
    return 0;
}

static int
fprint_float_literal(FloatLiteral* fl, FILE* fp) {
    FPRINTF(fp, "%.3f", fl->value);
    return 0;
}

static int
fprint_prefix_expression(PrefixExpression* pe, FILE* fp) {
    FPRINTF(fp, "(%s", pe->op);
    NODE_FPRINT(fp, pe->right);
    FPRINTF(fp, ")");
    return 0;
}

static int
fprint_infix_expression(InfixExpression* ie, FILE* fp) {
    FPRINTF(fp, "(");
    NODE_FPRINT(fp, ie->left);
    FPRINTF(fp, " %s ", ie->op);
    NODE_FPRINT(fp, ie->right);
    FPRINTF(fp, ")");
    return 0;
}

int fprint_block_statement(BlockStatement* bs, FILE* fp) {
    for (size_t i = 0; i < bs->len; i++) {
        NODE_FPRINT(fp, bs->stmts[i]);
    }
    return 0;
}

static int
fprint_if_expression(IfExpression* ie, FILE* fp) {
    FPRINTF(fp, "if");
    NODE_FPRINT(fp, ie->condition);
    FPRINTF(fp, " ");
    fprint_block_statement(ie->consequence, fp);
    if (ie->alternative != NULL) {
        FPRINTF(fp, "else ");
        fprint_block_statement(ie->alternative, fp);
    }
    return 0;
}

static int
fprint_function_literal(FunctionLiteral* fl, FILE* fp) {
    FPRINTF(fp, "%s(", fl->tok.literal);
    for (size_t i = 0; i < fl->len - 1; i++) {
        fprint_identifier(fl->params[i], fp);
        FPRINTF(fp, ", ");
    }
    if (fl->len > 0)
        fprint_identifier(fl->params[fl->len - 1], fp);
    FPRINTF(fp, ") ");
    fprint_block_statement(fl->body, fp);
    return 0;
}

static int
fprint_call_expression(CallExpression* ce, FILE* fp) {
    NODE_FPRINT(fp, ce->function);
    FPRINTF(fp, "(");
    for (size_t i = 0; i < ce->len - 1; i++) {
        NODE_FPRINT(fp, ce->args[i]);
        FPRINTF(fp, ", ");
    }
    if (ce->len > 0)
        NODE_FPRINT(fp, ce->args[ce->len - 1]);
    FPRINTF(fp, ")");
    return 0;
}

static int
fprint_let_statement(LetStatement* ls, FILE* fp) {
    FPRINTF(fp, "%s %s = ",
            ls->tok.literal, ls->name->value);
    if (ls->value.obj != NULL) {
        NODE_FPRINT(fp, ls->value);
    }
    FPRINTF(fp, ";");
    return 0;
}

static int
fprint_boolean(BooleanLiteral* b, FILE* fp) {
    FPRINTF(fp, "%s", b->tok.literal);
    return 0;
}

static int
fprint_return_statement(ReturnStatement* rs, FILE* fp) {
    FPRINTF(fp, "%s ", rs->tok.literal);
    if (rs->return_value.obj != NULL) {
        NODE_FPRINT(fp, rs->return_value);
    }
    FPRINTF(fp, ";");
    return 0;
}

static int
fprint_expression_statement(ExpressionStatement* es, FILE* fp) {
    NODE_FPRINT(fp, es->expression);
    return 0;
}

int node_fprint(const Node n, FILE* fp) {
    if (n.obj == NULL) return 0;

    switch (n.typ) {
        case n_Identifier:
            return fprint_identifier(n.obj, fp);

        case n_IntegerLiteral:
            return fprint_integer_literal(n.obj, fp);

        case n_FloatLiteral:
            return fprint_float_literal(n.obj, fp);

        case n_PrefixExpression:
            return fprint_prefix_expression(n.obj, fp);

        case n_InfixExpression:
            return fprint_infix_expression(n.obj, fp);

        case n_IfExpression:
            return fprint_if_expression(n.obj, fp);

        case n_FunctionLiteral:
            return fprint_function_literal(n.obj, fp);

        case n_CallExpression:
            return fprint_call_expression(n.obj, fp);

        case n_LetStatement:
            return fprint_let_statement(n.obj, fp);

        case n_BooleanLiteral:
            return fprint_boolean(n.obj, fp);

        case n_ReturnStatement:
            return fprint_return_statement(n.obj, fp);

        case n_ExpressionStatement:
            return fprint_expression_statement(n.obj, fp);

        case n_BlockStatement:
            return fprint_block_statement(n.obj, fp);

        default:
            fprintf(stderr, "node_fprint: node type not handled %d\n", n.typ);
            exit(1);
    }
}

static void
destroy_prefix_expression(PrefixExpression* pe) {
    free(pe->tok.literal);
    node_destroy(pe->right);
    free(pe);
}

static void
destroy_infix_expression(InfixExpression* ie) {
    node_destroy(ie->left);
    free(ie->op);
    node_destroy(ie->right);
    free(ie);
}

void destroy_block_statement(BlockStatement* bs) {
    free(bs->tok.literal);
    for (size_t i = 0; i < bs->len; i++)
        node_destroy(bs->stmts[i]);
    free(bs->stmts);
    free(bs);
}

static void
destroy_if_expression(IfExpression* ie) {
    free(ie->tok.literal);
    node_destroy(ie->condition);
    if (ie->consequence != NULL)
        destroy_block_statement(ie->consequence);
    if (ie->alternative != NULL)
        destroy_block_statement(ie->alternative);
    free(ie);
}

static void
destroy_function_literal(FunctionLiteral* fl) {
    free(fl->tok.literal);
    if (fl->params != NULL) {
        for (size_t i = 0; i < fl->len; i++) {
            free(fl->params[i]->value);
            free(fl->params[i]);
        }
        free(fl->params);
    }
    if (fl->body != NULL)
        destroy_block_statement(fl->body);
    free(fl);
}

static void
destroy_call_expression(CallExpression* ce) {
    free(ce->tok.literal);
    node_destroy(ce->function);
    for (size_t i = 0; i < ce->len; i++) {
        node_destroy(ce->args[i]);
    }
    free(ce->args);
    free(ce);
}

static void
destroy_let_statement(LetStatement* ls) {
    free(ls->tok.literal);
    node_destroy(ls->value);
    if (ls->name != NULL) {
        free(ls->name->tok.literal);
        free(ls->name);
    }
    free(ls);
}

static void
destroy_return_statement(ReturnStatement* rs) {
    free(rs->tok.literal);
    node_destroy(rs->return_value);
    free(rs);
}

static void
destroy_expression_statement(ExpressionStatement* es) {
    node_destroy(es->expression);
    free(es);
}

void node_destroy(Node n) {
    if (n.obj == NULL) return;

    switch (n.typ) {
        case n_PrefixExpression:
            return destroy_prefix_expression(n.obj);

        case n_InfixExpression:
            return destroy_infix_expression(n.obj);

        case n_IfExpression:
            return destroy_if_expression(n.obj);

        case n_FunctionLiteral:
            return destroy_function_literal(n.obj);

        case n_CallExpression:
            return destroy_call_expression(n.obj);

        case n_LetStatement:
            return destroy_let_statement(n.obj);

        case n_ReturnStatement:
            return destroy_return_statement(n.obj);

        case n_ExpressionStatement:
            return destroy_expression_statement(n.obj);

        case n_BlockStatement:
            return destroy_block_statement(n.obj);

        default:
            // since first field of `n.obj` is `Token`
            free(((Token*)n.obj)->literal);
            free(n.obj);
            break;
    }
}

int program_fprint(Program* p, FILE* fp) {
    Node* stmt;
    for (size_t i = 0; i < p->stmts.len; i++) {
        stmt = buffer_nth(&p->stmts, i);
        if (node_fprint(*stmt, fp) == -1)
            return -1;
    }
    return 0;
}
