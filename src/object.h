#pragma once

// This module defines types for Object.  A C variable of that type contains an
// ObjectType and ObjectData of the following categories:
//
// Primitive data types: nothing, integers and floats and booleans are stored
// entirely in ObjectData, so copies are deep copies.
//
// Compound data types: strings, arrays, tables (hashmaps) and closures contain
// pointers.  All copies are shallow, unless with the copy() builtin function
// (which wraps around object_copy).
//
// All functions (except builtins) are Closures and all Closures contain a free
// list, which contains shallow copies of variables defined outside the current
// function that are not global.

#include "ast.h"
#include "code.h"
#include "constants.h"
#include "errors.h"

#include <stdio.h>
#include <stdbool.h>

typedef enum __attribute__ ((__packed__)) {
    // Primitive data types:
    o_Nothing,
    o_Integer,
    o_Float,
    o_Boolean,
    o_BuiltinFunction,

    // A wrapper around `error`
    o_Error,

    // Compound data types:
    o_String,
    o_Array,
    o_Table,
    o_Closure,
} ObjectType;

struct Object;
struct Table;
struct Closure;
struct Builtin;
BUFFER(Object, struct Object)
BUFFER(Char, char)

typedef union {
    long integer;
    double floating;
    bool boolean;
    const struct Builtin *builtin;
    error err;

    CharBuffer *string;
    ObjectBuffer *array;
    struct Table *table;
    struct Closure *closure;

    void *ptr; // generic pointer
} ObjectData;

typedef struct Object {
    ObjectType type;
    ObjectData data;
} Object;

typedef struct Closure {
    CompiledFunction *func;

    int num_free;
    Object free[];
} Closure;

#define OBJ(t, d) (Object){ .type = t, .data = { d } }
#define OBJ_ERR(...) OBJ(o_Error, .err = errorf(__VA_ARGS__))
#define OBJ_BOOL(b) OBJ(o_Boolean, .boolean = b)
#define OBJ_NOTHING (Object){0}

bool is_truthy(Object obj);

// compare [left] and [right]. returns [Null Object] otherwise
Object object_eq(Object left, Object right);

// print [Object] to [fp], returns -1 on error
int object_fprint(Object o, FILE* fp);

const char* show_object_type(ObjectType t);
