#pragma once

#include "ast.h"

#include <stdio.h>
#include <stdbool.h>

#define OBJ(t, d) (Object){ t, false, {d} }
#define NULL_OBJ() (Object){} // `typ` == 0 == o_Null
#define BOOL(b) (Object){ o_Boolean, false, {.boolean = b} }

#define IS_ERROR(obj) obj.typ == o_Error
#define IS_NULL(obj) obj.typ == o_Null

typedef enum __attribute__ ((__packed__)) {
    // Primitives (not heap allocated)
    o_Null, // TODO? `null` tokens, lexing and parsing?
    o_Integer,
    o_Float,
    o_Boolean,
    o_ReturnValue, // if `ObjectType` is `o_ReturnValue`
                   // `Object` is cast into `struct ReturnValue` with
                   // `to_return_value`.
    // Heap allocated
    o_Error,
    o_Function,
} ObjectType;

// defined in "environment.h"
typedef struct Function Function;

typedef union {
    long integer;
    double floating;
    bool boolean;
    char* error_msg;
    Function* func;
} ObjectData;

typedef struct {
    ObjectType typ;
    bool is_marked;
    ObjectData data;
} Object;

BUFFER(Object, Object);

int object_fprint(Object o, FILE* fp);
const char* show_object_type(ObjectType t);
void object_destroy(Object* o);
bool object_cmp(Object left, Object right);

// stores the actual type of `value` in second byte
struct ReturnValue {
    ObjectType typ; // will be `o_ReturnValue`
    bool is_marked;
    ObjectType value_typ;
    ObjectData value;
};

Object to_return_value(Object obj);
Object from_return_value(Object obj);
