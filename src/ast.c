#include "ast.h"
#include "token.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NODE_FPRINT(fp, ...) \
    if (node_fprint( __VA_ARGS__, fp) == -1) \
        return -1;

DEFINE_BUFFER(Param, Identifier*);
DEFINE_BUFFER(Node, Node);
DEFINE_BUFFER(Pair, Pair);

char* token_literal(const Node n) {
    // since the first element of every `node.obj` is a token.
    Token* tok = n.obj;
    return tok->literal;
}

static int
fprint_identifier(Identifier* i, FILE* fp) {
    FPRINTF(fp, "%s", i->tok.literal);
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
    for (int i = 0; i < bs->stmts.length; i++) {
        NODE_FPRINT(fp, bs->stmts.data[i]);
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
    FPRINTF(fp, "%s", fl->tok.literal);
    if (fl->name != NULL) {
        FPRINTF(fp, "<%s>", fl->name);
    }
    FPRINTF(fp, "(");
    for (int i = 0; i < fl->params.length - 1; i++) {
        fprint_identifier(fl->params.data[i], fp);
        FPRINTF(fp, ", ");
    }
    if (fl->params.length > 0)
        fprint_identifier(fl->params.data[0], fp);
    FPRINTF(fp, ") ");
    fprint_block_statement(fl->body, fp);
    return 0;
}

static int
fprint_call_expression(CallExpression* ce, FILE* fp) {
    NODE_FPRINT(fp, ce->function);
    FPRINTF(fp, "(");
    for (int i = 0; i < ce->args.length - 1; i++) {
        NODE_FPRINT(fp, ce->args.data[i]);
        FPRINTF(fp, ", ");
    }
    if (ce->args.length > 0)
        NODE_FPRINT(fp, ce->args.data[ce->args.length - 1]);
    FPRINTF(fp, ")");
    return 0;
}

static int
fprint_let_statement(LetStatement* ls, FILE* fp) {
    FPRINTF(fp, "%s %s = ",
            ls->tok.literal, ls->name->tok.literal);
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
    FPRINTF(fp, ";");
    return 0;
}

static int
fprint_array_literal(ArrayLiteral* al, FILE* fp) {
    FPRINTF(fp, "[");
    for (int i = 0; i < al->elements.length - 1; i++) {
        NODE_FPRINT(fp, al->elements.data[i]);
        FPRINTF(fp, ", ");
    }
    if (al->elements.length > 0)
        NODE_FPRINT(fp, al->elements.data[al->elements.length - 1]);
    FPRINTF(fp, "]");
    return 0;
}

static int
fprint_index_expression(IndexExpression* ie, FILE* fp) {
    NODE_FPRINT(fp, ie->left);
    FPRINTF(fp, "[");
    NODE_FPRINT(fp, ie->index);
    FPRINTF(fp, "]");
    return 0;
}

static int
fprint_hash_literal_entry(Node key, Node value, FILE* fp) {
    NODE_FPRINT(fp, key);
    FPRINTF(fp, ": ");
    NODE_FPRINT(fp, value);
    return 0;
}

static int
fprint_hash_literal(HashLiteral* hl, FILE* fp) {
    FPRINTF(fp, "{");
    for (int i = 0; i < hl->pairs.length - 1; i++) {
        Pair* pair = &hl->pairs.data[i];
        fprint_hash_literal_entry(pair->key, pair->val, fp);
        FPRINTF(fp, ", ");
    }
    if (hl->pairs.length > 1) {
        Pair* pair = &hl->pairs.data[hl->pairs.length - 1];
        fprint_hash_literal_entry(pair->key, pair->val, fp);
    }
    FPRINTF(fp, "}");
    return 0;
}

static int
fprint_string_literal(StringLiteral* sl, FILE* fp) {
    FPRINTF(fp, "\"%s\"", sl->tok.literal);
    return 0;
}

static int
fprint_assign_expression(AssignExpression *ae, FILE *fp) {
    node_fprint(ae->left, fp);
    FPRINTF(fp, " = ");
    node_fprint(ae->right, fp);
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

        case n_ArrayLiteral:
            return fprint_array_literal(n.obj, fp);

        case n_IndexExpression:
            return fprint_index_expression(n.obj, fp);

        case n_HashLiteral:
            return fprint_hash_literal(n.obj, fp);

        case n_StringLiteral:
            return fprint_string_literal(n.obj, fp);

        case n_AssignExpression:
            return fprint_assign_expression(n.obj, fp);

        case n_NullLiteral:
            FPRINTF(fp, "null");
            return 0;

        default:
            fprintf(stderr, "node_fprint: node type not handled %d\n", n.typ);
            exit(1);
    }
}

static void
free_prefix_expression(PrefixExpression* pe) {
    free(pe->tok.literal);
    node_free(pe->right);
    free(pe);
}

static void
free_infix_expression(InfixExpression* ie) {
    node_free(ie->left);
    free(ie->op);
    node_free(ie->right);
    free(ie);
}

void free_block_statement(BlockStatement* bs) {
    free(bs->tok.literal);
    for (int i = 0; i < bs->stmts.length; i++) {
        node_free(bs->stmts.data[i]);
    }
    free(bs->stmts.data);
    free(bs);
}

static void
free_if_expression(IfExpression* ie) {
    free(ie->tok.literal);
    node_free(ie->condition);
    if (ie->consequence != NULL)
        free_block_statement(ie->consequence);
    if (ie->alternative != NULL)
        free_block_statement(ie->alternative);
    free(ie);
}

void free_function_literal(FunctionLiteral* fl) {
    free(fl->tok.literal);
    for (int i = 0; i < fl->params.length; i++) {
        free(fl->params.data[i]->tok.literal);
        free(fl->params.data[i]);
    }
    free(fl->params.data);
    if (fl->body != NULL)
        free_block_statement(fl->body);
    free(fl);
}

static void
free_call_expression(CallExpression* ce) {
    free(ce->tok.literal);
    node_free(ce->function);
    if (ce->args.data != NULL) {
        for (int i = 0; i < ce->args.length; i++) {
            node_free(ce->args.data[i]);
        }
        free(ce->args.data);
    }
    free(ce);
}

static void
free_let_statement(LetStatement* ls) {
    free(ls->tok.literal);
    if (ls->name != NULL) {
        free(ls->name->tok.literal);
        free(ls->name);
    }
    node_free(ls->value);
    free(ls);
}

static void
free_return_statement(ReturnStatement* rs) {
    free(rs->tok.literal);
    node_free(rs->return_value);
    free(rs);
}

static void
free_expression_statement(ExpressionStatement* es) {
    node_free(es->expression);
    free(es);
}

static void
free_array_literal(ArrayLiteral* al) {
    free(al->tok.literal);
    if (al->elements.data != NULL) {
        for (int i = 0; i < al->elements.length; i++)
            node_free(al->elements.data[i]);
        free(al->elements.data);
    }
    free(al);
}

static void
free_index_expression(IndexExpression* ie) {
    free(ie->tok.literal);
    node_free(ie->left);
    node_free(ie->index);
    free(ie);
}

void free_hash_literal(HashLiteral* hl) {
    free(hl->tok.literal);
    for (int i = 0; i < hl->pairs.length; i++) {
        Pair* pair = &hl->pairs.data[i];
        node_free(pair->key);
        node_free(pair->val);
    }
    free(hl->pairs.data);
    free(hl);
}

static void
free_assign_expression(AssignExpression *ae) {
    node_free(ae->left);
    node_free(ae->right);
    free(ae->tok.literal);
    free(ae);
}

void node_free(Node n) {
    if (n.obj == NULL) return;

    switch (n.typ) {
        case n_PrefixExpression:
            return free_prefix_expression(n.obj);

        case n_InfixExpression:
            return free_infix_expression(n.obj);

        case n_IfExpression:
            return free_if_expression(n.obj);

        case n_FunctionLiteral:
            return free_function_literal(n.obj);

        case n_CallExpression:
            return free_call_expression(n.obj);

        case n_LetStatement:
            return free_let_statement(n.obj);

        case n_ReturnStatement:
            return free_return_statement(n.obj);

        case n_ExpressionStatement:
            return free_expression_statement(n.obj);

        case n_BlockStatement:
            return free_block_statement(n.obj);

        case n_ArrayLiteral:
            return free_array_literal(n.obj);

        case n_IndexExpression:
            return free_index_expression(n.obj);

        case n_AssignExpression:
            return free_assign_expression(n.obj);

        case n_HashLiteral:
            return free_hash_literal(n.obj);

        default:
            // since first field of `n.obj` is `Token`
            free(((Token*)n.obj)->literal);
            free(n.obj);
            break;
    }
}

int program_fprint(Program* p, FILE* fp) {
    for (int i = 0; i < p->stmts.length; i++) {
        if (node_fprint(p->stmts.data[i], fp) == -1)
            return -1;
    }
    return 0;
}
