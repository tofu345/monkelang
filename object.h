#ifndef OBJECT_H
#define OBJECT_H

// my DRY senses are tingling
//
// TODO: evaluate using ast Node's directly

#include <stdio.h>
#include <stdbool.h>

enum ObjectType {
    o_Integer = 1,
    o_Boolean,
    o_Null,
};

typedef struct {
    enum ObjectType typ;
    void* obj;
} Object;

int object_fprint(Object o, FILE* fp);
void object_destroy(Object* o);
const char* show_object_type(enum ObjectType t);

typedef struct {
    long value;
} Integer;

typedef struct {
    bool value;
} Boolean;

struct Null {};

#endif
