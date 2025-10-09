#pragma once

#include "ast.h"

#include <stdio.h>
#include <stdbool.h>

typedef enum __attribute__ ((__packed__)) {
    // Primitives (concrete values or ptrs do not need freeing by GC)
    o_Null, // TODO? `null` tokens, lexing and parsing?
    o_Integer,
    o_Float,
    o_Boolean,

    // Heap Allocated Objects
    o_String,
    o_Error,
    o_Array,
    o_Hash,
} ObjectType;

typedef struct Object Object;
BUFFER(Object, Object);
BUFFER(Char, char);

// typedef struct Closure Closure;
// typedef struct Env Env;
// typedef Object* Builtin(Env* env, ObjectBuffer* args);

typedef union {
    long integer;
    double floating;
    bool boolean;

    CharBuffer* string; // [o_String]
    ObjectBuffer* array;
    ht* hash;
} ObjectData;

struct Object {
    ObjectType type;
    ObjectData data;
};

const char* show_object_type(ObjectType t);
int object_fprint(Object o, FILE* fp);
bool object_eq(Object left, Object right);
