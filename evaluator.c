#include "evaluator.h"
#include "ast.h"
#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Object eval(Node n);

#define _BOOL(b) (Object){ o_Boolean, {.boolean = b} }
#define STR_EQ(exp, str) strcmp(exp, str) == 0

static Object
eval_integer_literal(IntegerLiteral* il) {
    return (Object){ o_Integer, {il->value} };
}

static Object
eval_float_literal(FloatLiteral* fl) {
    return (Object){ o_Float, {fl->value} };
}

static Object
eval_statements(Node* stmt, size_t len) {
    Object result = {};
    for (size_t i = 0; i < len; i++) {
        result = eval(stmt[i]);
    }
    return result;
}

static Object
eval_minus_prefix_operator_expression(Object right) {
    if (right.typ == o_Integer) {
        return (Object){ o_Integer, {-right.data.integer} };
    } else if (right.typ == o_Float) {
        return (Object){ o_Float, {-right.data.floating} };
    } else
        return (Object){};
    return right;
}

static Object
eval_bang_operator_expression(Object right) {
    if (right.typ == o_Boolean) {
        return _BOOL(!right.data.boolean);

    } else if (right.typ == o_Null) {
        return _BOOL(true);
    }
    return _BOOL(false);
}

static Object
eval_prefix_expression(PrefixExpression* pe) {
    Object right = eval(pe->right);

    if (STR_EQ("!", pe->op)) {
        return eval_bang_operator_expression(right);

    } else if (STR_EQ("-", pe->op)) {
        return eval_minus_prefix_operator_expression(right);
    }

    return (Object){};
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
        return _BOOL(left_val < right_val);

    } else if (STR_EQ(">", op)) {
        return _BOOL(left_val > right_val);

    } else if (STR_EQ("==", op)) {
        return _BOOL(left_val == right_val);

    } else if (STR_EQ("!=", op)) {
        return _BOOL(left_val != right_val);

    } else {
        return (Object){};
    }

    return (Object){ o_Float, {left_val} };
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
        return _BOOL(left_val < right_val);

    } else if (STR_EQ(">", op)) {
        return _BOOL(left_val > right_val);

    } else if (STR_EQ("==", op)) {
        return _BOOL(left_val == right_val);

    } else if (STR_EQ("!=", op)) {
        return _BOOL(left_val != right_val);

    } else {
        return (Object){};
    }

    return (Object){ o_Integer, {left_val} };
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
        return _BOOL(left.data.boolean == right.data.boolean);

    } else if (STR_EQ("!=", ie->op)) {
        return _BOOL(left.data.boolean != right.data.boolean);
    }
    return (Object){};
}

Object eval(Node n) {
    switch (n.typ) {
        case n_IntegerLiteral:
            return eval_integer_literal(n.obj);
        case n_FloatLiteral:
            return eval_float_literal(n.obj);
        case n_BooleanLiteral:
            {
                BooleanLiteral* b = n.obj;
                return _BOOL(b->value);
            }
        case n_PrefixExpression:
            return eval_prefix_expression(n.obj);
        case n_InfixExpression:
            return eval_infix_expression(n.obj);
        case n_ExpressionStatement:
            return eval(
                    ((ExpressionStatement*)n.obj)->expression);
        default:
            return (Object){};
    }
}

Object eval_program(const Program* p) {
    return eval_statements(p->stmts, p->len);
}
