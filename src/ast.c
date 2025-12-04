#include "ast.h"
#include "token.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *node_token(Node n) {
    return n.obj;
}

#define NODE_FPRINT(fp, ...) \
    if (node_fprint( __VA_ARGS__, fp) == -1) \
        return -1;

#define FPRINTF_TOKEN(fp, tok) \
    if (fprintf(fp, "%.*s", tok.length, tok.start) <= 0) \
        return -1;

DEFINE_BUFFER(Param, Identifier*)
DEFINE_BUFFER(Node, Node)
DEFINE_BUFFER(Pair, Pair)

static int
fprint_identifier(Identifier* i, FILE* fp) {
    FPRINTF_TOKEN(fp, i->tok)
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
    FPRINTF(fp, "(");
    FPRINTF_TOKEN(fp, pe->tok);
    NODE_FPRINT(fp, pe->right);
    FPRINTF(fp, ")");
    return 0;
}

static int
fprint_infix_expression(InfixExpression* ie, FILE* fp) {
    FPRINTF(fp, "(");
    NODE_FPRINT(fp, ie->left);
    FPRINTF(fp, " ");
    FPRINTF_TOKEN(fp, ie->tok)
    FPRINTF(fp, " ");
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
    FPRINTF(fp, "if ");
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
    FPRINTF_TOKEN(fp, fl->tok);
    if (fl->name != NULL) {
        Identifier *id = fl->name;
        FPRINTF(fp, "<%.*s>", id->tok.length, id->tok.start);
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
    FPRINTF(fp, "let ")
    FPRINTF_TOKEN(fp, ls->name->tok)
    FPRINTF(fp, " = ")
    if (ls->value.obj != NULL) {
        NODE_FPRINT(fp, ls->value);
    }
    FPRINTF(fp, ";");
    return 0;
}

static int
fprint_boolean(BooleanLiteral* b, FILE* fp) {
    FPRINTF_TOKEN(fp, b->tok)
    return 0;
}

static int
fprint_return_statement(ReturnStatement* rs, FILE* fp) {
    FPRINTF_TOKEN(fp, rs->tok)
    FPRINTF(fp, " ")
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
fprint_for_statement(ForStatement* fs, FILE* fp) {
    FPRINTF(fp, "for (");
    NODE_FPRINT(fp, fs->init);
    NODE_FPRINT(fp, fs->condition);
    FPRINTF(fp, ";");
    NODE_FPRINT(fp, fs->update);
    FPRINTF(fp, ")");
    node_fprint(NODE(n_BlockStatement, fs->body), fp);
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
fprint_table_literal_entry(Node key, Node value, FILE* fp) {
    NODE_FPRINT(fp, key);
    FPRINTF(fp, ": ");
    NODE_FPRINT(fp, value);
    return 0;
}

static int
fprint_table_literal(TableLiteral* hl, FILE* fp) {
    FPRINTF(fp, "{");
    for (int i = 0; i < hl->pairs.length - 1; i++) {
        Pair* pair = &hl->pairs.data[i];
        fprint_table_literal_entry(pair->key, pair->val, fp);
        FPRINTF(fp, ", ");
    }
    if (hl->pairs.length > 1) {
        Pair* pair = &hl->pairs.data[hl->pairs.length - 1];
        fprint_table_literal_entry(pair->key, pair->val, fp);
    }
    FPRINTF(fp, "}");
    return 0;
}

static int
fprint_string_literal(StringLiteral* sl, FILE* fp) {
    FPRINTF(fp, "\"");
    FPRINTF_TOKEN(fp, sl->tok);
    FPRINTF(fp, "\"");
    return 0;
}

static int
fprint_assignment(Assignment *as, FILE *fp) {
    node_fprint(as->left, fp);
    FPRINTF(fp, " = ");
    node_fprint(as->right, fp);
    FPRINTF(fp, ";");
    return 0;
}

static int
fprint_operator_assignment(OperatorAssignment *stmt, FILE *fp) {
    node_fprint(stmt->left, fp);
    FPRINTF(fp, " %.*s= ", LITERAL(stmt->tok));
    node_fprint(stmt->right, fp);
    FPRINTF(fp, ";");
    return 0;
}

int node_fprint(const Node n, __attribute__ ((unused)) FILE* fp) {
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

        case n_ForStatement:
            return fprint_for_statement(n.obj, fp);

        case n_ArrayLiteral:
            return fprint_array_literal(n.obj, fp);

        case n_IndexExpression:
            return fprint_index_expression(n.obj, fp);

        case n_TableLiteral:
            return fprint_table_literal(n.obj, fp);

        case n_StringLiteral:
            return fprint_string_literal(n.obj, fp);

        case n_Assignment:
            return fprint_assignment(n.obj, fp);

        case n_OperatorAssignment:
            return fprint_operator_assignment(n.obj, fp);

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
    node_free(pe->right);
    free(pe);
}

static void
free_infix_expression(InfixExpression* ie) {
    node_free(ie->left);
    node_free(ie->right);
    free(ie);
}

void free_block_statement(BlockStatement* bs) {
    for (int i = 0; i < bs->stmts.length; i++) {
        node_free(bs->stmts.data[i]);
    }
    free(bs->stmts.data);
    free(bs);
}

static void
free_if_expression(IfExpression* ie) {
    node_free(ie->condition);
    if (ie->consequence != NULL)
        free_block_statement(ie->consequence);
    if (ie->alternative != NULL)
        free_block_statement(ie->alternative);
    free(ie);
}

void free_function_literal(FunctionLiteral* fl) {
    for (int i = 0; i < fl->params.length; i++) {
        free(fl->params.data[i]);
    }
    free(fl->params.data);
    if (fl->body != NULL)
        free_block_statement(fl->body);
    free(fl);
}

static void
free_call_expression(CallExpression* ce) {
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
    if (ls->name != NULL) {
        free(ls->name);
    }
    node_free(ls->value);
    free(ls);
}

static void
free_return_statement(ReturnStatement* rs) {
    node_free(rs->return_value);
    free(rs);
}

static void
free_expression_statement(ExpressionStatement* es) {
    node_free(es->expression);
    free(es);
}

void free_for_statement(ForStatement *fs) {
    node_free(fs->init);
    node_free(fs->condition);
    node_free(fs->update);
    if (fs->body) {
        free_block_statement(fs->body);
    }
    free(fs);
}

static void
free_array_literal(ArrayLiteral* al) {
    if (al->elements.data != NULL) {
        for (int i = 0; i < al->elements.length; i++)
            node_free(al->elements.data[i]);
        free(al->elements.data);
    }
    free(al);
}

static void
free_index_expression(IndexExpression* ie) {
    node_free(ie->left);
    node_free(ie->index);
    free(ie);
}

void free_table_literal(TableLiteral* hl) {
    for (int i = 0; i < hl->pairs.length; i++) {
        Pair* pair = &hl->pairs.data[i];
        node_free(pair->key);
        node_free(pair->val);
    }
    free(hl->pairs.data);
    free(hl);
}

static void
free_assignment(Assignment *as) {
    node_free(as->left);
    node_free(as->right);
    free(as);
}

static void
free_operator_assignment(OperatorAssignment *as) {
    node_free(as->left);
    node_free(as->right);
    free(as);
}

void node_free(Node n) {
    if (n.obj == NULL) return;

    switch (n.typ) {
        case n_PrefixExpression:
            free_prefix_expression(n.obj);
            return;

        case n_InfixExpression:
            free_infix_expression(n.obj);
            return;

        case n_IfExpression:
            free_if_expression(n.obj);
            return;

        case n_FunctionLiteral:
            free_function_literal(n.obj);
            return;

        case n_CallExpression:
            free_call_expression(n.obj);
            return;

        case n_LetStatement:
            free_let_statement(n.obj);
            return;

        case n_ReturnStatement:
            free_return_statement(n.obj);
            return;

        case n_ExpressionStatement:
            free_expression_statement(n.obj);
            return;

        case n_ForStatement:
            free_for_statement(n.obj);
            return;

        case n_BlockStatement:
            free_block_statement(n.obj);
            return;

        case n_ArrayLiteral:
            free_array_literal(n.obj);
            return;

        case n_IndexExpression:
            free_index_expression(n.obj);
            return;

        case n_Assignment:
            free_assignment(n.obj);
            return;

        case n_OperatorAssignment:
            free_operator_assignment(n.obj);
            return;

        case n_TableLiteral:
            free_table_literal(n.obj);
            return;

        default:
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
