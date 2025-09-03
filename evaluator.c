#include "evaluator.h"
#include "ast.h"
#include "environment.h"
#include "object.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Object eval(Node n, Env* env);

#define STR_EQ(exp, str) strcmp(exp, str) == 0

Object new_error(char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    if (vasprintf(&msg, format, args) == -1) {
        ALLOC_FAIL();
    };
    va_end(args);
    return OBJ(o_Error, .error_msg = msg);
}

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
eval_block_statement(NodeBuffer stmts, Env* env) {
    Object result = {};
    for (int i = 0; i < stmts.length; i++) {
        result = eval(stmts.data[i], env);
        switch (result.typ) {
            case o_ReturnValue: return result;
            case o_Error: return result;
            default: break;
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
            return new_error("unknown operator: -%s",
                    show_object_type(right.typ));
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
eval_prefix_expression(PrefixExpression* pe, Env* env) {
    Object right = eval(pe->right, env);
    if (IS_ERROR(right))
        return right;

    if (STR_EQ("!", pe->op)) {
        return eval_bang_operator_expression(right);

    } else if (STR_EQ("-", pe->op)) {
        return eval_minus_prefix_operator_expression(right);

    } else {
        return new_error("unknown operator: %s%s",
                pe->op, show_object_type(right.typ));
    }
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
        return new_error("unknown operator: %s %s %s",
                show_object_type(left.typ), op,
                show_object_type(right.typ));
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
        return new_error("unknown operator: %s %s %s",
                show_object_type(left.typ), op,
                show_object_type(right.typ));
    }

    return OBJ(o_Integer, left_val);
}

static Object
eval_infix_expression(InfixExpression* ie, Env* env) {
    Object left = eval(ie->left, env);
    if (IS_ERROR(left))
        return left;

    Object right = eval(ie->right, env);
    if (IS_ERROR(right))
        return right;

    if (left.typ == o_Integer && right.typ == o_Integer) {
        return eval_integer_infix_expression(ie->op, left, right);

    } else if (left.typ == o_Float || right.typ == o_Float) {
        return eval_float_infix_expression(ie->op, left, right);

    } else if (STR_EQ("==", ie->op)) {
        return BOOL(object_cmp(left, right));

    } else if (STR_EQ("!=", ie->op)) {
        return BOOL(!object_cmp(left, right));

    } else if (left.typ != right.typ) {
        return new_error("type mismatch: %s %s %s",
                show_object_type(left.typ), ie->op,
                show_object_type(right.typ));

    } else {
        return new_error("unknown operator: %s %s %s",
                show_object_type(left.typ), ie->op,
                show_object_type(right.typ));
    }
}

static Object
eval_if_expression(IfExpression* ie, Env* env) {
    Object condition = eval(ie->condition, env);
    if (is_truthy(condition)) {
        return eval_block_statement(ie->consequence->stmts, env);

    } else if (ie->alternative != NULL) {
        return eval_block_statement(ie->alternative->stmts, env);
    }

    return NULL_OBJ();
}

static Object
eval_identifier(Identifier* i, Env* env) {
    Object val = env_get(env, i->value);
    if (IS_NULL(val)) {
        return new_error("identifier not found: %s", i->value);
    }
    return val;
}

static ObjectBuffer
eval_argument_expressions(NodeBuffer args, Env* env) {
    ObjectBuffer results;
    ObjectBufferInit(&results);
    for (int i = 0; i < args.length; i++) {
        Object result = eval(args.data[i], env);
        if (IS_ERROR(result)) {
            for (int j = 0; j < i; i++)
                object_destroy(&results.data[j]);

            results.length = 1;
            results.data[0] = result;
            return results;
        }
        ObjectBufferPush(&results, result);
    }
    return results;
}

// load args into new frame, run function,
// remove new frame and return result
static Object
apply_function(Object fn, ObjectBuffer args, Env* env) {
    if (fn.typ != o_Function)
        return new_error("not a function: %s",
                show_object_type(fn.typ));

    FunctionLiteral* func = fn.data.func;
    Frame* func_frame = frame_new(env);
    func_frame->parent = env->current;
    env->current = func_frame;
    // is there need to create ObjectBuffer?
    for (int i = 0; i < args.length; i++)
        env_set(env, func->params.data[i]->value, args.data[i]);

    Object result = eval_block_statement(func->body->stmts, env);
    env->current = func_frame->parent;

    // if [result] is a [FunctionLiteral], [result] is a Closure

    frame_destroy(func_frame, env);

    // mark_and_sweep(env);

    if (result.typ == o_ReturnValue)
        return from_return_value(result);
    return result;
}

Object eval(Node n, Env* env) {
    switch (n.typ) {
        case n_Identifier:
            return eval_identifier(n.obj, env);
        case n_IfExpression:
            return eval_if_expression(n.obj, env);
        case n_IntegerLiteral:
            return eval_integer_literal(n.obj);
        case n_FloatLiteral:
            return eval_float_literal(n.obj);
        case n_BooleanLiteral:
            {
                BooleanLiteral* b = n.obj;
                return BOOL(b->value);
            }
        case n_FunctionLiteral:
            return OBJ(o_Function, .func = n.obj);
        case n_PrefixExpression:
            return eval_prefix_expression(n.obj, env);
        case n_InfixExpression:
            return eval_infix_expression(n.obj, env);
        case n_CallExpression:
            {
                CallExpression* ce = n.obj;
                Object function = eval(ce->function, env);
                if (IS_ERROR(function)) {
                    return function;
                }
                ObjectBuffer args = eval_argument_expressions(ce->args, env);
                if (args.length == 1 && IS_ERROR(args.data[0])) {
                    Object err = args.data[0];
                    free(args.data);
                    return err;
                }
                Object result = apply_function(function, args, env);
                for (int i = 0; i < args.length; i++)
                    object_destroy(&args.data[i]);
                free(args.data);
                return result;
            }
        case n_LetStatement:
            {
                LetStatement* ls = n.obj;
                Object val = eval(ls->value, env);
                if (IS_ERROR(val)) {
                    return val;
                }
                env_set(env, ls->name->value, val);
                return NULL_OBJ();
            }
        case n_BlockStatement:
            {
                BlockStatement* b = n.obj;
                return eval_block_statement(b->stmts, env);
            }
        case n_ReturnStatement:
            {
                ReturnStatement* ns = n.obj;
                Object res = eval(ns->return_value, env);
                if (IS_ERROR(res)) {
                    return res;
                }
                return to_return_value(res);
            }
        case n_ExpressionStatement:
            {
                ExpressionStatement* es = n.obj;
                return eval(es->expression, env);
            }
        default:
            return NULL_OBJ();
    }
}


Object eval_program(Program* p, Env* env) {
    Object result = {};
    for (int i = 0; i < p->stmts.length; i++) {
        result = eval(p->stmts.data[i], env);
        switch (result.typ) {
            case o_ReturnValue: return from_return_value(result);
            case o_Error: return result;
            default: break;
        }
    }
    return result;
}
