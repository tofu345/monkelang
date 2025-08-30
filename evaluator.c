#include "evaluator.h"
#include "ast.h"
#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Object eval(Node n);

#define OBJ(t, d) (Object){ t, {d} }
#define NULL_OBJ(b) (Object){} // `typ` == 0 == o_Null
#define BOOL(b) (Object){ o_Boolean, {.boolean = b} }

#define STR_EQ(exp, str) strcmp(exp, str) == 0

bool is_truthy(Object o) {
    switch (o.typ) {
        case o_Null: return false;
        case o_Boolean: return o.data.boolean;
        default: return true; // TODO? c-like?
    }
}

static Object
eval_integer_literal(IntegerLiteral* il) {
    return OBJ(o_Integer, il->value);
}

static Object
eval_float_literal(FloatLiteral* fl) {
    return OBJ(o_Float, fl->value);
}

static Object
eval_block_statement(Node* stmt, size_t len) {
    Object result = {};
    for (size_t i = 0; i < len; i++) {
        result = eval(stmt[i]);
        if (result.typ == o_ReturnValue) {
            return result;
        }
    }
    return result;
}

static Object
eval_minus_prefix_operator_expression(Object right) {
    switch (right.typ) {
        case o_Integer:
            return OBJ(o_Integer, -right.data.integer);
        case o_Float:
            return OBJ(o_Float, -right.data.floating);
        default:
            return NULL_OBJ();
    }
}

static Object
eval_bang_operator_expression(Object right) {
    switch (right.typ) {
        case o_Boolean:
            return BOOL(!right.data.boolean);
        case o_Null:
            return BOOL(true);
        default:
            return BOOL(false);
    }
}

static Object
eval_prefix_expression(PrefixExpression* pe) {
    Object right = eval(pe->right);

    if (STR_EQ("!", pe->op)) {
        return eval_bang_operator_expression(right);

    } else if (STR_EQ("-", pe->op)) {
        return eval_minus_prefix_operator_expression(right);
    }

    return NULL_OBJ();
}

static double
conv_int_to_float(Object obj) {
    if (obj.typ == o_Integer)
        return (double)obj.data.integer;

    else if (obj.typ == o_Float)
        return obj.data.floating;

    else {
        printf("cannot convert type %s to float\n", show_object_type(obj.typ));
        exit(1);
    }
}

static Object
eval_float_infix_expression(char* op, Object left, Object right) {
    double left_val = conv_int_to_float(left);
    double right_val = conv_int_to_float(right);

    if (STR_EQ("+", op)) {
        left_val += right_val;

    } else if (STR_EQ("-", op)) {
        left_val -= right_val;

    } else if (STR_EQ("*", op)) {
        left_val *= right_val;

    } else if (STR_EQ("/", op)) {
        left_val /= right_val;

    } else if (STR_EQ("<", op)) {
        return BOOL(left_val < right_val);

    } else if (STR_EQ(">", op)) {
        return BOOL(left_val > right_val);

    } else if (STR_EQ("==", op)) {
        return BOOL(left_val == right_val);

    } else if (STR_EQ("!=", op)) {
        return BOOL(left_val != right_val);

    } else {
        return NULL_OBJ();
    }

    return OBJ(o_Float, left_val);
}

static Object
eval_integer_infix_expression(char* op, Object left, Object right) {
    long left_val = left.data.integer;
    long right_val = right.data.integer;

    if (STR_EQ("+", op)) {
        left_val += right_val;

    } else if (STR_EQ("-", op)) {
        left_val -= right_val;

    } else if (STR_EQ("*", op)) {
        left_val *= right_val;

    } else if (STR_EQ("/", op)) {
        left_val /= right_val;

    } else if (STR_EQ("<", op)) {
        return BOOL(left_val < right_val);

    } else if (STR_EQ(">", op)) {
        return BOOL(left_val > right_val);

    } else if (STR_EQ("==", op)) {
        return BOOL(left_val == right_val);

    } else if (STR_EQ("!=", op)) {
        return BOOL(left_val != right_val);

    } else {
        return NULL_OBJ();
    }

    return OBJ(o_Integer, left_val);
}

static Object
eval_infix_expression(InfixExpression* ie) {
    Object left = eval(ie->left);
    Object right = eval(ie->right);

    if (left.typ == o_Integer && right.typ == o_Integer) {
        return eval_integer_infix_expression(ie->op, left, right);

    } else if (left.typ == o_Float || right.typ == o_Float) {
        return eval_float_infix_expression(ie->op, left, right);

    } else if (STR_EQ("==", ie->op)) {
        return BOOL(left.data.boolean == right.data.boolean);

    } else if (STR_EQ("!=", ie->op)) {
        return BOOL(left.data.boolean != right.data.boolean);
    }

    return NULL_OBJ();
}

static Object
eval_if_expression(IfExpression* ie) {
    Object condition = eval(ie->condition);
    if (is_truthy(condition)) {
        return eval((Node){ n_BlockStatement, ie->consequence });

    } else if (ie->alternative != NULL) {
        return eval((Node){ n_BlockStatement, ie->alternative });
    }

    return NULL_OBJ();
}

Object eval(Node n) {
    switch (n.typ) {
        case n_BlockStatement:
            {
                BlockStatement* b = n.obj;
                return eval_block_statement(b->statements, b->len);
            }
        case n_IfExpression:
            return eval_if_expression(n.obj);
        case n_IntegerLiteral:
            return eval_integer_literal(n.obj);
        case n_FloatLiteral:
            return eval_float_literal(n.obj);
        case n_BooleanLiteral:
            {
                BooleanLiteral* b = n.obj;
                return BOOL(b->value);
            }
        case n_PrefixExpression:
            return eval_prefix_expression(n.obj);
        case n_InfixExpression:
            return eval_infix_expression(n.obj);
        case n_ReturnStatement:
            {
                ReturnStatement* ns = n.obj;
                return to_return_value(eval(ns->return_value));
            }
        case n_ExpressionStatement:
            {
                ExpressionStatement* es = n.obj;
                return eval(es->expression);
            }
        default:
            return NULL_OBJ();
    }
}

Object eval_program(const Program* p) {
    Object result = {};
    for (size_t i = 0; i < p->len; i++) {
        result = eval(p->stmts[i]);
        if (result.typ == o_ReturnValue) {
            return from_return_value(result);
        }
    }
    return result;
}
