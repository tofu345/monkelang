#include "evaluator.h"
#include "ast.h"
#include "environment.h"
#include "object.h"
#include "utils.h"
#include "builtin.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const Object o_true = (Object){ o_Boolean, .data = {.boolean = true}};
const Object o_false = (Object){ o_Boolean, .data = {.boolean = false}};
const Object o_null = (Object){ o_Null };

#define TRUE (Object*)&o_true
#define FALSE (Object*)&o_false
#define NULL_OBJ() (Object*)&o_null

#define IS_ERROR(obj) (obj->typ == o_Error)
#define IS_NULL(obj) (obj == &o_null)
#define STR_EQ(exp, str) (strcmp(exp, str) == 0)

Object* eval(Env* env, Node n);
static ObjectBuffer eval_expressions(Env* env, NodeBuffer args);

Object* object_copy(Env* env, Object* obj) {
    switch (obj->typ) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_Error:
        case o_Function:
        case o_BuiltinFunction:
        case o_Closure:
            return object_new(env, obj->typ, obj->data);
        case o_String:
            {
                CharBuffer* new_str = allocate(sizeof(CharBuffer));
                *new_str = CharBufferCopy(obj->data.string);
                return OBJ(o_String, .string = new_str);
            }
        case o_Array:
            {
                ObjectBuffer* new_arr = allocate(sizeof(ObjectBuffer));
                *new_arr = ObjectBufferCopy(obj->data.array);
                for (int i = 0; i < new_arr->length; i++) {
                    new_arr->data[i] = object_copy(env, new_arr->data[i]);
                }
                return OBJ(o_Array, .array = new_arr);
            }
        case o_ReturnValue:
            fprintf(stderr, "object_copy: cannot copy ReturnValue\n");
            exit(1);
            return obj;
        default:
            fprintf(stderr, "object_copy: object type not handled %d\n",
                    obj->typ);
            exit(1);
    }
}

Object* new_error(Env* env, char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    if (vasprintf(&msg, format, args) == -1) {
        ALLOC_FAIL();
    };
    va_end(args);
    return OBJ(o_Error, .error_msg = msg);
}

bool is_truthy(Object* o) {
    switch (o->typ) {
        case o_Null: return false;
        case o_Boolean: return o->data.boolean;
        default: return true; // TODO? c-like?
    }
}

static Object*
eval_integer_literal(Env* env, IntegerLiteral* il) {
    return OBJ(o_Integer, il->value);
}

static Object*
eval_float_literal(Env* env, FloatLiteral* fl) {
    return OBJ(o_Float, .floating = fl->value);
}

static Object*
eval_string_literal(Env* env, StringLiteral* sl) {
    size_t len = strlen(sl->tok.literal);
    CharBuffer* str = allocate(sizeof(CharBuffer));
    CharBufferInit(str);
    if (len > 0) {
        str->capacity = power_of_2_ceil(len + 1);
        str->data = allocate(str->capacity * sizeof(char));
        memcpy(str->data, sl->tok.literal, len * sizeof(char));
        str->data[len] = '\0';
    }
    str->length = len;
    return OBJ(o_String, .string = str);
}

static Object*
eval_array_literal(Env* env, ArrayLiteral* al) {
    ObjectBuffer* arr = allocate(sizeof(ObjectBuffer));
    *arr = eval_expressions(env, al->elements);
    if (arr->length == 1 && IS_ERROR(arr->data[0])) {
        Object* err = arr->data[0];
        free(arr->data);
        return err;
    }
    return OBJ(o_Array, .array = arr);
}

static Object*
eval_block_statement(Env* env, NodeBuffer stmts) {
    Object* result = NULL_OBJ();
    for (int i = 0; i < stmts.length; i++) {
        result = eval(env, stmts.data[i]);
        switch (result->typ) {
            case o_ReturnValue: return result;
            case o_Error: return result;
            default: break;
        }
    }
    return result;
}

static Object*
eval_minus_prefix_operator_expression(Env* env, Object* right) {
    switch (right->typ) {
        case o_Integer:
            return OBJ(o_Integer, -right->data.integer);
        case o_Float:
            return OBJ(o_Float, -right->data.floating);
        default:
            return new_error(env, "unknown operator: -%s",
                    show_object_type(right->typ));
    }
}

static Object*
bool_to_object(bool value) {
    if (value) return TRUE;
    return FALSE;
}

static Object*
eval_bang_operator_expression(Object* right) {
    switch (right->typ) {
        case o_Boolean:
            return bool_to_object(!right->data.boolean);
        case o_Null:
            return TRUE;
        default:
            return FALSE;
    }
}

static Object*
eval_prefix_expression(Env* env, PrefixExpression* pe) {
    Object* right = eval(env, pe->right);
    if (IS_ERROR(right))
        return right;

    if (STR_EQ("!", pe->op)) {
        return eval_bang_operator_expression(right);

    } else if (STR_EQ("-", pe->op)) {
        return eval_minus_prefix_operator_expression(env, right);

    } else {
        return new_error(env, "unknown operator: %s%s",
                pe->op, show_object_type(right->typ));
    }
}

static double
conv_int_to_float(Object* obj) {
    if (obj->typ == o_Integer)
        return (double)obj->data.integer;

    else if (obj->typ == o_Float)
        return obj->data.floating;

    else {
        fprintf(stderr,
                "cannot convert type %s to float\n",
                show_object_type(obj->typ));
        exit(1);
    }
}

static Object*
eval_float_infix_expression(Env* env, char* op, Object* left, Object* right) {
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
        return bool_to_object(left_val < right_val);

    } else if (STR_EQ(">", op)) {
        return bool_to_object(left_val > right_val);

    } else if (STR_EQ("==", op)) {
        return bool_to_object(left_val == right_val);

    } else if (STR_EQ("!=", op)) {
        return bool_to_object(left_val != right_val);

    } else {
        return new_error(env, "unknown operator: %s %s %s",
                show_object_type(left->typ), op,
                show_object_type(right->typ));
    }

    return OBJ(o_Float, .floating = left_val);
}

static Object*
eval_string_infix_expression(Env* env, char* op, Object* left, Object* right) {
    if (!STR_EQ("+", op))
        return new_error(env, "unknown operator: %s %s %s",
                show_object_type(left->typ), op,
                show_object_type(right->typ));

    // this should go into a Buffer* fn ..
    CharBuffer* left_val = left->data.string;
    CharBuffer* right_val = right->data.string;
    CharBuffer* result = allocate(sizeof(CharBuffer));
    result->length = left_val->length + right_val->length;
    result->capacity = power_of_2_ceil(result->length + 1);
    result->data = allocate(result->capacity * sizeof(char));
    memcpy(result->data, left_val->data,
            left_val->length * sizeof(char));
    memcpy(result->data + left_val->length, right_val->data,
            right_val->length * sizeof(char));
    result->data[result->length] = '\0';

    return OBJ(o_String, .string = result);
}

static Object*
eval_integer_infix_expression(Env* env, char* op, Object* left, Object* right) {
    long left_val = left->data.integer;
    long right_val = right->data.integer;

    if (STR_EQ("+", op)) {
        left_val += right_val;

    } else if (STR_EQ("-", op)) {
        left_val -= right_val;

    } else if (STR_EQ("*", op)) {
        left_val *= right_val;

    } else if (STR_EQ("/", op)) {
        left_val /= right_val;

    } else if (STR_EQ("<", op)) {
        return bool_to_object(left_val < right_val);

    } else if (STR_EQ(">", op)) {
        return bool_to_object(left_val > right_val);

    } else if (STR_EQ("==", op)) {
        return bool_to_object(left_val == right_val);

    } else if (STR_EQ("!=", op)) {
        return bool_to_object(left_val != right_val);

    } else {
        return new_error(env, "unknown operator: %s %s %s",
                show_object_type(left->typ), op,
                show_object_type(right->typ));
    }

    return OBJ(o_Integer, left_val);
}

static Object*
eval_infix_expression(Env* env, InfixExpression* ie) {
    Object* left = eval(env, ie->left);
    if (IS_ERROR(left))
        return left;

    track(env, left);
    Object* right = eval(env, ie->right);
    untrack(env, 1);
    if (IS_ERROR(right))
        return right;

    if (left->typ == o_Integer && right->typ == o_Integer) {
        return eval_integer_infix_expression(env, ie->op, left, right);

    } else if (left->typ == o_Float || right->typ == o_Float) {
        return eval_float_infix_expression(env, ie->op, left, right);

    } else if (left->typ == o_String && right->typ == o_String) {
        return eval_string_infix_expression(env, ie->op, left, right);

    } else if (STR_EQ("==", ie->op)) {
        return bool_to_object(object_eq(left, right));

    } else if (STR_EQ("!=", ie->op)) {
        return bool_to_object(!object_eq(left, right));

    } else if (left->typ != right->typ) {
        return new_error(env, "type mismatch: %s %s %s",
                show_object_type(left->typ), ie->op,
                show_object_type(right->typ));

    } else {
        return new_error(env, "unknown operator: %s %s %s",
                show_object_type(left->typ), ie->op,
                show_object_type(right->typ));
    }
}

static Object*
eval_if_expression(Env* env, IfExpression* ie) {
    Object* condition = eval(env, ie->condition);
    if (is_truthy(condition)) {
        return eval_block_statement(env, ie->consequence->stmts);

    } else if (ie->alternative != NULL) {
        return eval_block_statement(env, ie->alternative->stmts);
    }

    return NULL_OBJ();
}

static Object*
eval_identifier(Env* env, Identifier* i) {
    Object* val = env_get(env, i->tok.literal);
    if (val != NULL)
        return val;

    Object* builtin = builtin_get(env, i->tok.literal);
    if (builtin != NULL)
        return builtin;

    return new_error(env, "identifier not found: %s", i->tok.literal);
}

static ObjectBuffer
eval_expressions(Env* env, NodeBuffer args) {
    ObjectBuffer results;
    ObjectBufferInit(&results);

    for (int i = 0; i < args.length; i++) {
        Object* result = eval(env, args.data[i]);
        if (IS_ERROR(result)) {
            untrack(env, i);

            if (results.length == 0)
                ObjectBufferPush(&results, result);
            else {
                results.length = 1;
                results.data[0] = result;
            }
            return results;
        }
        ObjectBufferPush(&results, result);
        track(env, result); // keep in scope
    }

    untrack(env, args.length);

    return results;
}

// append all [Identifiers] in Node to [buf]
static void
captures_in(Node n, ParamBuffer* buf) {
    switch (n.typ) {
        case n_Identifier:
            return ParamBufferPush(buf, n.obj);
        case n_PrefixExpression:
            {
                PrefixExpression* pe = n.obj;
                captures_in(pe->right, buf);
                return;
            }
        case n_InfixExpression:
            {
                InfixExpression* ie = n.obj;
                captures_in(ie->left, buf);
                captures_in(ie->right, buf);
                return;
            }
        case n_IfExpression:
            {
                IfExpression* ie = n.obj;
                captures_in(ie->condition, buf);
                captures_in(NODE(n_BlockStatement, ie->consequence), buf);
                if (ie->alternative != NULL)
                    captures_in(
                            NODE(n_BlockStatement, ie->alternative), buf);
                return;
            }
        case n_FunctionLiteral:
            {
                FunctionLiteral* fl = n.obj;
                captures_in(NODE(n_BlockStatement, fl->body), buf);
                for (int i = 0; i < fl->params.length; i++)
                    ParamBufferPush(buf, fl->params.data[i]);
                return;
            }
        case n_CallExpression:
            {
                CallExpression* ce = n.obj;
                for (int i = 0; i < ce->args.length; i++)
                    captures_in(ce->args.data[i], buf);
                return;
            }
        case n_LetStatement:
            {
                LetStatement* ls = n.obj;
                captures_in(ls->value, buf);
                return;
            }
        case n_ReturnStatement:
            {
                ReturnStatement* rs = n.obj;
                captures_in(rs->return_value, buf);
                return;
            }
        case n_ExpressionStatement:
            {
                ExpressionStatement* es = n.obj;
                captures_in(es->expression, buf);
                return;
            }
        case n_BlockStatement:
            {
                BlockStatement* bs = n.obj;
                for (int i = 0; i < bs->stmts.length; i++)
                    captures_in(bs->stmts.data[i], buf);
                return;
            }
        default: return;
    }
}

// load args into new frame, run function,
// remove new frame and return result
static Object*
apply_function(Env* env, Object* fn, ObjectBuffer* args, char* fn_name) {
    FunctionLiteral* func; // which fn to run
    Object* result;
    Frame *previous = env->current,
          *new_frame = frame_new(env);

    if (fn->typ == o_Function) {
        func = fn->data.func;
        new_frame->parent = env->current;
        env->current = new_frame;

    } else if (fn->typ == o_Closure) {
        func = fn->data.closure->func;
        env->current = new_frame;
        new_frame->parent = fn->data.closure->frame;

    } else {
        fprintf(stderr,
                "apply_function: function type %s not handled\n",
                show_object_type(fn->typ));
        exit(1);
    }

    if (func->params.length != args->length)
        return new_error(env,
                "function %s() takes %d argument%s got %d",
                fn_name, func->params.length,
                func->params.length != 1 ? "s" : "", args->length);

    for (int i = 0; i < args->length; i++) {
        env_set(env,
                func->params.data[i]->tok.literal,
                object_copy(env, args->data[i]));
    }

    result = from_return_value(eval_block_statement(env, func->body->stmts));

    // [result] is a Closure
    if (result->typ == o_Function) {
        ParamBuffer captured_variables;
        ParamBufferInit(&captured_variables);
        captures_in(
                NODE(n_BlockStatement, result->data.func->body),
                &captured_variables);

        // copy [captured_variables] into new [Frame] for Closure
        Frame* closure_frame = frame_new(env);
        for (int i = 0; i < captured_variables.length; i++) {
            char* name = captured_variables.data[i]->tok.literal;

            // get from evaluated function Frame.
            Object* value = env_get(env, name);
            if (value == NULL) continue;

            // insert in closure frame.
            if (ht_set(closure_frame->table, name, value) == NULL)
                ALLOC_FAIL();
        }
        free(captured_variables.data);

        Closure* closure = allocate(sizeof(Closure));
        closure->frame = closure_frame;
        closure->func = result->data.func;
        result = OBJ(o_Closure, .closure = closure );
    }

    env->current = previous;
    frame_destroy(new_frame, env);
    if (!IS_NULL(result))
        trace_mark_object(result);
    mark_and_sweep(env);

    return result;
}

static Object*
eval_call_expression(Env* env, CallExpression* ce) {
    char* fn_name = ce->function.typ == n_Identifier
        ? ((Identifier*)ce->function.obj)->tok.literal
        : "<anonymous>";
    Object *result,
           *function = eval(env, ce->function);
    ObjectBuffer args = eval_expressions(env, ce->args);
    if (args.length == 1 && IS_ERROR(args.data[0])) {
        Object* err = args.data[0];
        free(args.data);
        return err;
    }
    switch (function->typ) {
        case o_Error:
            result = function;
            break;
        case o_Function:
        case o_Closure:
            result = apply_function(env, function, &args, fn_name);
            break;
        case o_BuiltinFunction:
            {
                result = function->data.builtin(env, &args);
                if (result == NULL) result = NULL_OBJ();
                break;
            }
        default:
            result = new_error(env, "not a function: %s",
                    show_object_type(function->typ));
    }
    free(args.data);
    return result;
}

static Object*
eval_array_index_expression(Object* left, Object* index) {
    ObjectBuffer* arr = left->data.array;
    long idx = index->data.integer;
    long max = arr->length - 1;
    if (idx < 0 || idx > max) return NULL_OBJ();
    return arr->data[idx];
}

static char*
hash_key(Object* key) {
    switch (key->typ) {
        case o_String: return key->data.string->data;
        default: return NULL;
    }
}

static Object*
eval_hash_index_expression(Env* env, Object* left, Object* index) {
    ht* pairs = left->data.hash;

    char* key = hash_key(index);
    if (key == NULL) {
        return new_error(env, "unusable as hash key: %s",
                show_object_type(index->typ));
    }

    Object* val = ht_get(pairs, key);
    if (val == NULL) {
        return NULL_OBJ();
    }

    return val;
}

static Object*
eval_index_expression(Env* env, Object* left, Object* index) {
    if (left->typ == o_Array && index->typ == o_Integer) {
        return eval_array_index_expression(left, index);
    } else if (left->typ == o_Hash) {
        return eval_hash_index_expression(env, left, index);
    } else {
        return new_error(env, "index operator not supported for '%s[%s]'",
                show_object_type(left->typ), show_object_type(index->typ));
    }
}

static Object*
eval_hash_literal(Env* env, HashLiteral* hl) {
    ht* pairs = ht_create();
    for (int i = 0; i < hl->pairs.length; i++) {
        Object* key = eval(env, hl->pairs.data[i].key);
        if (IS_ERROR(key) || IS_NULL(key)) {
            ht_destroy(pairs);
            return key;
        }

        char* h_key = hash_key(key);
        if (h_key == NULL) {
            ht_destroy(pairs);
            return new_error(env, "unusable as hash key: %s",
                    show_object_type(key->typ));
        }

        Object* value = eval(env, hl->pairs.data[i].val);
        if (IS_ERROR(value) || IS_NULL(value)) {
            ht_destroy(pairs);
            return value;
        }

        if (ht_set(pairs, h_key, value) == NULL) ALLOC_FAIL();
    }

    return OBJ(o_Hash, .hash = pairs);
}

Object* eval(Env* env, Node n) {
    switch (n.typ) {
        case n_Identifier:
            return eval_identifier(env, n.obj);
        case n_IfExpression:
            return eval_if_expression(env, n.obj);
        case n_IntegerLiteral:
            return eval_integer_literal(env, n.obj);
        case n_FloatLiteral:
            return eval_float_literal(env, n.obj);
        case n_BooleanLiteral:
            {
                BooleanLiteral* b = n.obj;
                return bool_to_object(b->value);
            }
        case n_StringLiteral:
            return eval_string_literal(env, n.obj);
        case n_ArrayLiteral:
            return eval_array_literal(env, n.obj);
        case n_IndexExpression:
            {
                IndexExpression* ie = n.obj;
                Object* left = eval(env, ie->left);
                if (IS_ERROR(left)) {
                    return left;
                }
                Object* index = eval(env, ie->index);
                if (IS_ERROR(index)) {
                    return index;
                }
                return eval_index_expression(env, left, index);
            }
        case n_FunctionLiteral:
            return OBJ(o_Function, .func = n.obj);
        case n_HashLiteral:
            return eval_hash_literal(env, n.obj);
        case n_PrefixExpression:
            return eval_prefix_expression(env, n.obj);
        case n_InfixExpression:
            return eval_infix_expression(env, n.obj);
        case n_CallExpression:
            return eval_call_expression(env, n.obj);
        case n_LetStatement:
            {
                LetStatement* ls = n.obj;
                Object* val = eval(env, ls->value);
                if (IS_ERROR(val)) {
                    return val;
                }
                env_set(env, ls->name->tok.literal, val);
                return NULL_OBJ();
            }
        case n_BlockStatement:
            {
                BlockStatement* b = n.obj;
                return eval_block_statement(env, b->stmts);
            }
        case n_ReturnStatement:
            {
                ReturnStatement* ns = n.obj;
                Object* res = eval(env, ns->return_value);
                if (IS_ERROR(res)) {
                    return res;
                }
                return to_return_value(res);
            }
        case n_ExpressionStatement:
            {
                ExpressionStatement* es = n.obj;
                return eval(env, es->expression);
            }
        default:
            return NULL_OBJ();
    }
}

Object* eval_program(Program* p, Env* env) {
    Object* result = NULL_OBJ();
    for (int i = 0; i < p->stmts.length; i++) {
        result = eval(env, p->stmts.data[i]);
        switch (result->typ) {
            case o_ReturnValue:
                return from_return_value(result);
            case o_Error:
                return result;
            default: break;
        }
    }
    return result;
}
