#pragma once

#include "ast.h"

#include <stdio.h>
#include <stdbool.h>

typedef enum __attribute__ ((__packed__)) {
    o_Null, // TODO? `null` tokens, lexing and parsing?
    o_Integer,
    o_Float,
    o_Boolean,
    o_ReturnValue, // if [ObjectType] is [o_ReturnValue] [Object] is cast into
                   // [struct ReturnValue] with `to_return_value()`.
    o_BuiltinFunction,
    o_Error,
    o_Function,
    o_Closure, // a [Frame] with captured variables, to keep them
               // in scope.
    o_String,
    o_Array,
} ObjectType;

typedef struct Object Object;
BUFFER(Object, Object*);

typedef struct Closure Closure;
typedef struct Env Env;
typedef Object* Builtin(Env* env, ObjectBuffer* args);

BUFFER(Char, char);

typedef union {
    long integer;
    double floating;
    bool boolean;
    char* error_msg;
    FunctionLiteral* func;
    Closure* closure;
    Builtin* builtin;
    CharBuffer* string;
    ObjectBuffer* array;
} ObjectData;

struct Object {
    ObjectType typ;
    bool is_marked;
    ObjectData data;
};

const char* show_object_type(ObjectType t);
int object_fprint(Object* o, FILE* fp);
void object_destroy(Object* o);
bool object_eq(Object* left, Object* right);

// stores the actual type of `value` in [value_typ]
struct ReturnValue {
    ObjectType typ; // will be `o_ReturnValue`
    bool is_marked;
    ObjectType value_typ;
    ObjectData value;
};

Object* to_return_value(Object* obj);
Object* from_return_value(Object* obj);
