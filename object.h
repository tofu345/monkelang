#pragma once

#include <stdio.h>
#include <stdbool.h>

typedef enum __attribute__ ((__packed__)) {
    // Primitives (not heap allocated)
    o_Null, // TODO? `null` tokens, lexing and parsing?
    o_Integer,
    o_Float,
    o_Boolean,
    o_ReturnValue, // if `ObjectType` is `o_ReturnValue`
                   // `Object` is cast into `struct ReturnValue`.
    // Heap allocated (uses `.ptr`)
} ObjectType;

typedef union {
    long integer;
    double floating;
    bool boolean;
    void* ptr;
} ObjectData;

typedef struct {
    ObjectType typ;
    ObjectData data;
} Object;

int object_fprint(Object o, FILE* fp);
const char* show_object_type(ObjectType t);
void object_destroy(Object* o);

// stores the actual type of `value` in second byte
struct ReturnValue {
    ObjectType typ; // will be `o_ReturnValue`
    ObjectType value_typ;
    ObjectData value;
};

Object to_return_value(Object obj);
Object from_return_value(Object obj);
