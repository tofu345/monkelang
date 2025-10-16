#pragma once

#include "ast.h"
#include "code.h"
#include "constants.h"

#include <stdio.h>
#include <stdbool.h>

// TODO:
// - char datatype
// - Hash -> Table, CompiledFunction -> Function
// - show_object_type() lowercase type names
// - support Unicode and emoji's

#define OBJ(t, d) (Object){ .type = t, .data = { d } }
#define ERR(...) OBJ(o_Error, .err = new_error(__VA_ARGS__))
#define BOOL(b) OBJ(o_Boolean, .boolean = b)
#define NULL_OBJ (Object){}

#define IS_ERR(obj) (obj.type == o_Error)

typedef struct Object Object;
BUFFER(Object, Object);
BUFFER(Char, char);
typedef struct table table;

typedef enum __attribute__ ((__packed__)) {
    // Primitive data types:
    // Data types that do not need garbage collection
    o_Null,
    o_Integer,
    o_Float,
    o_Boolean,
    o_CompiledFunction,
    o_BuiltinFunction,
    o_Error,

    // Compound data types:
    o_String,
    o_Array,
    o_Hash,
} ObjectType;

typedef union {
    long integer;
    double floating;
    bool boolean;

    CharBuffer* string;
    ObjectBuffer* array;
    table* hash;
    CompiledFunction *func;
    error err;
    void *ptr;
} ObjectData;

struct Object {
    ObjectType type;
    ObjectData data;
};

const char* show_object_type(ObjectType t);

// can return error
Object object_eq(Object left, Object right);

// print [Object] to [fp], returns -1 on error
int object_fprint(Object o, FILE* fp);
