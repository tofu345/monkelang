#pragma once

// This module defines types for Object.  A C variable of that type contains an
// ObjectType and ObjectData of the following categories:
//
// Primitive data types: null, numbers and booleans are stored entirely
// in ObjectData, copying them copies their value.
//
// Compound data types: strings, arrays, tables (hashmaps) and closures contain
// pointers.  Copying them only copies their pointer.
//
// All functions are Closures and all Closures contain a list of Objects
// representing its free variables, variables defined outside the
// current function that are not global variables.  Because of this,
// assignments to free variables of primitive types only change the
// value in the free variable list.

#include "ast.h"
#include "code.h"
#include "constants.h"
#include "errors.h"

#include <stdio.h>
#include <stdbool.h>

#define OBJ(t, d) (Object){ .type = t, .data = { d } }
#define ERR(...) OBJ(o_Error, .err = new_error(__VA_ARGS__))
#define BOOL(b) OBJ(o_Boolean, .boolean = b)
#define NULL_OBJ (Object){ .type = 0 }

#define IS_ERR(obj) (obj.type == o_Error)
#define IS_NULL(obj) (obj.type == o_Null)

typedef struct Object Object;
typedef struct Table Table;
typedef struct Closure Closure;
typedef struct Builtin Builtin;

BUFFER(Object, Object)
BUFFER(Char, char)

typedef enum __attribute__ ((__packed__)) {
    // Primitive data types:
    o_Null,
    o_Integer,
    o_Float,
    o_Boolean,
    o_BuiltinFunction,

    // A wrapper around [error], which is simply a [char *]
    o_Error,

    // Compound data types:
    o_String,
    o_Array,
    o_Table,
    o_Closure,
} ObjectType;

typedef union {
    long integer;
    double floating;
    bool boolean;
    const Builtin *builtin;
    error err;

    CharBuffer* string;
    ObjectBuffer* array;
    Table* table;
    Closure *closure;

    void *ptr; // generic pointer
} ObjectData;

struct Object {
    ObjectType type;
    ObjectData data;
};

struct Closure {
    CompiledFunction *func;

    int num_free;
    Object free[];
};

bool is_truthy(Object obj);

// compare [left] and [right]. returns [Null Object] otherwise
Object object_eq(Object left, Object right);

// print [Object] to [fp], returns -1 on error
int object_fprint(Object o, FILE* fp);

const char* show_object_type(ObjectType t);
