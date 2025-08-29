#pragma once

// garbage collection based on [boot.dev: c memory mgmt course](https://www.youtube.com/watch?v=rJrd2QMVbGM)

#include <stdio.h>
#include <stdbool.h>

struct Null {};

typedef enum __attribute__ ((__packed__)) {
    o_Integer = 1,
    o_Float,
    o_Boolean,
    o_Null, // last object type that cannot contain references to
            // another `Object`.
} ObjectType;

typedef union {
    long integer;
    double floating;
    bool boolean;
    void* null;
} ObjectData;

typedef struct {
    ObjectType typ; // err if 0
    ObjectData data;
} Object;

int object_fprint(Object o, FILE* fp);
const char* show_object_type(ObjectType t);
void object_destroy(Object* o);
