#pragma once

#include <stddef.h>

#include "ast.h"
#include "buffer.h"
#include "hash-table/ht.h"
#include "object.h"

// garbage collector (mark and sweep).
// based on [boot.dev: c memory mgmt course](https://www.youtube.com/watch?v=rJrd2QMVbGM)
//
// Runs when:
// - an allocation fails
// - a function returns

struct HeapObject {
    ObjectType typ;
    void* ptr;
};

BUFFER(Frame, ht*);
BUFFER(Alloc, struct HeapObject);

typedef struct {
    FrameBuffer frames;
    AllocBuffer allocs; // heap allocated [Objects]
} Env;

struct Function {
    ParamBuffer params;
    BlockStatement* body;
    Env* env;
    FunctionLiteral* lit; // [params] and [body] point to the same
                          // value as `FunctionLiteral`. When the
                          // result of an evaluation is `Function`,
                          // set NULL on `FunctionLiteral`.
};

// `malloc` new `Object`, store ptr in [env.allocs] and return ptr
Object* allocate_object(Env* env);

Env* env_new();
void* enclosed_env(Env* env);
void env_destroy(Env* env);

Object env_get(Env* e, char* name);

// copies obj into env
void* env_set(Env* e, char* name, Object obj);
