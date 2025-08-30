#pragma once

#include <stdio.h>
#include <stdbool.h>

struct Null {};

typedef enum __attribute__ ((__packed__)) {
    // Primitives (not heap allocated)
    o_Null,
    o_Integer,
    o_Float,
    o_Boolean,
    // Heap allocated
} ObjectType;

typedef union {
    long integer;
    double floating;
    bool boolean;
    void* ptr;
} ObjectData;

typedef struct {
    ObjectType typ; // err if 0
    ObjectData data;
} Object;

int object_fprint(Object o, FILE* fp);
const char* show_object_type(ObjectType t);
void object_destroy(Object* o);
