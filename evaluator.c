#include "evaluator.h"
#include "ast.h"
#include "object.h"

#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _BOOL(b) (Object){ o_Boolean, b }
#define _NULL() (Object){ o_Null, Null }
#define STR_EQ(exp, str) strcmp(exp, str) == 0

// no `const` because of warnings :|
void* True = &(Boolean){ true };
void* False = &(Boolean){ false };
void* Null = &(struct Null){};

static Object
eval_integer_literal(IntegerLiteral* il) {
    Integer* i = malloc(sizeof(Integer));
    i->value = il->value;
    return (Object){ o_Integer, i };
}

static Object
native_bool_to_boolean_object(bool value) {
    if (value)
        return _BOOL(True);
    return _BOOL(False);
}

static Object
eval_statements(Node* stmt, size_t len) {
    Object result = {};
    for (size_t i = 0; i < len; i++) {
        if (result.obj != NULL)
            object_destroy(&result); // free previous objects
        result = eval(stmt[i]);
    }
    return result;
}

static Object
eval_minus_prefix_operator_expression(Object right) {
    if (right.typ != o_Integer) {
        return _NULL();
    }

    Integer* i = right.obj;
    i->value = -i->value;
    return right;
}

static Object
eval_bang_operator_expression(Object right) {
    if (right.obj == True) {
        return _BOOL(False);
    } else if (right.obj == False) {
        return _BOOL(True);
    } else if (right.obj == Null) {
        return _BOOL(True);
    } else
        return _BOOL(False);
}

static Object
eval_prefix_expression(char* op, Object right) {
    if (STR_EQ("!", op)) {
        return eval_bang_operator_expression(right);
    } else if (STR_EQ("-", op)) {
        return eval_minus_prefix_operator_expression(right);
    }
    return _NULL();
}

static Object
eval_integer_infix_expression(char* op, Object left, Object right) {
    Integer* left_int = left.obj;
    long left_val = left_int->value;
    long right_val = ((Integer*)right.obj)->value;

    if (STR_EQ("+", op)) {
        left_int->value = left_val + right_val;
        return left;

    } else if (STR_EQ("-", op)) {
        left_int->value = left_val - right_val;
        return left;

    } else if (STR_EQ("*", op)) {
        left_int->value = left_val * right_val;
        return left;

    } else if (STR_EQ("/", op)) {
        left_int->value = left_val / right_val;
        return left;

    } else if (STR_EQ("<", op)) {
        return native_bool_to_boolean_object(left_val < right_val);

    } else if (STR_EQ(">", op)) {
        return native_bool_to_boolean_object(left_val > right_val);

    } else if (STR_EQ("==", op)) {
        return native_bool_to_boolean_object(left_val == right_val);

    } else if (STR_EQ("!=", op)) {
        return native_bool_to_boolean_object(left_val != right_val);

    } else {
        return _NULL();
    }
}

static Object
eval_infix_expression(char* op, Object left, Object right) {
    if (left.typ == o_Integer && right.typ == o_Integer) {
        return eval_integer_infix_expression(op, left, right);
    }
    return _NULL();
}

Object eval(Node n) {
    switch (n.typ) {
        case n_IntegerLiteral:
            return eval_integer_literal(n.obj);
        case n_BooleanLiteral:
            return native_bool_to_boolean_object(
                    ((BooleanLiteral*)n.obj)->value);
        case n_PrefixExpression:
            {
                PrefixExpression* pe = n.obj;
                Object right = eval(pe->right);
                return eval_prefix_expression(pe->op, right);
            }
        case n_InfixExpression:
            {
                InfixExpression* ie = n.obj;
                Object left = eval(ie->left);
                Object right = eval(ie->right);
                return eval_infix_expression(ie->op, left, right);
            }
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
