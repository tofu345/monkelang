#pragma once

#include "ast.h"

#include <stdio.h>
#include <stdbool.h>

// TODO o_Error, `null` token, lexing and parsing

#define OBJ(t, d) (Object){ .type = t, .data = { d } }

typedef struct Object Object;
BUFFER(Object, Object);
BUFFER(Char, char);
typedef struct table table;

typedef enum __attribute__ ((__packed__)) {
    // Primitive data types:
    o_Null,
    o_Integer,
    o_Float,
    o_Boolean,

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
    void *ptr;
} ObjectData;

struct Object {
    ObjectType type;
    ObjectData data;
};

const char* show_object_type(ObjectType t);
bool object_eq(Object left, Object right);

// print [Object] to [fp], returns -1 on error
int object_fprint(Object o, FILE* fp);
