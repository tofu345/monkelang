#pragma once

#include "ast.h"
#include "code.h"
#include "constants.h"
#include "errors.h"

#include <stdio.h>
#include <stdbool.h>

// TODO:
// - char datatype
// - support Unicode and emoji's
// - stack tracing
// - store [Node] in [Symbol] instead of name?

#define OBJ(t, d) (Object){ .type = t, .data = { d } }
#define ERR(...) OBJ(o_Error, .err = new_error(__VA_ARGS__))
#define BOOL(b) OBJ(o_Boolean, .boolean = b)
#define NULL_OBJ (Object){ .type = 0 }

#define IS_ERR(obj) (obj.type == o_Error)
#define IS_NULL(obj) (obj.type == o_Null)

typedef struct Object Object;
typedef struct table table;
typedef struct Closure Closure;
typedef struct Builtin Builtin;

BUFFER(Object, Object)
BUFFER(Char, char)

typedef enum __attribute__ ((__packed__)) {
    // Primitive data types:
    // Data types that do not need garbage collection
    o_Null,
    o_Integer,
    o_Float,
    o_Boolean,
    o_BuiltinFunction,
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

    CharBuffer* string;
    ObjectBuffer* array;
    table* table;
    Closure *closure;
    error err;
    const Builtin *builtin;

    void *ptr;
} ObjectData;

struct Object {
    ObjectType type;
    ObjectData data;
};

const char* show_object_type(ObjectType t);

// compare [left] and [right]. returns error otherwise
Object object_eq(Object left, Object right);

// print [Object] to [fp], returns -1 on error
int object_fprint(Object o, FILE* fp);

struct Closure {
    Function *func;

    int num_free;
    Object free[]; // free variables, see [symbol_table.h/FreeScope]
};
