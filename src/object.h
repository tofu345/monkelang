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
    o_Error, // A wrapper around `error`

    // Compound data types:
    o_String,
    o_Array,
    o_Table,
    o_Closure,
    o_Module, // NOTE: for now, not exposed to end users.
              // Only Present in ObjectType to be used in vm `Frame`.
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
    struct Module *module;

    void *ptr; // generic pointer
} ObjectData;

typedef struct Object {
    ObjectType type;
    ObjectData data;
} Object;

#define OBJ(t, d) (Object){ .type = t, .data = { d } }
#define OBJ_ERR(...) OBJ(o_Error, .err = errorf(__VA_ARGS__))
#define OBJ_BOOL(b) OBJ(o_Boolean, .boolean = b)
#define OBJ_NOTHING (Object){0}

bool is_truthy(Object obj);
Object object_eq(Object left, Object right);

// print `Object` to `FILE *`, returns -1 on error
int object_fprint(Object, FILE *);

const char* show_object_type(ObjectType t);

// A Compiled FunctionLiteral.
typedef struct {
    Instructions instructions;
    int num_locals;
    int num_parameters;

    // Where in the source code the function was defined.
    FunctionLiteral *literal;

    // [SourceMapping] for all statements in [literal.body].
    SourceMappingBuffer mappings;
} CompiledFunction;

void free_function(CompiledFunction *fn);

typedef struct Closure {
    CompiledFunction *func;

    int num_free;
    Object free[];
} Closure;

int fprint_closure(Closure *cl, FILE *fp);
