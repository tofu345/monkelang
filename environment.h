#pragma once

#include <stddef.h>

#include "hash-table/ht.h"
#include "object.h"

// garbage collector (mark and sweep).
//
// based on [boot.dev: c memory mgmt course](https://www.youtube.com/watch?v=rJrd2QMVbGM)
//
// - Stores pointers to all allocated objects.
//
// Plan to run when:
// - to run when an allocation fails
// - when a function returns
typedef struct {
    void* ptrs; // allocated `Object` pointers
    size_t len;
    size_t cap;
} GC;

// wrapper around malloc that will run GC when malloc fails
// void* allocate();

struct Env {
    ht* store;
    struct Env* outer;
};
typedef struct Env Env;

struct Function {
    Identifier** params;
    size_t params_len;
    BlockStatement* body;
    Env* env;
};

Env* env_new();
Env* env_enclosed_new(Env* outer);
void env_destroy(Env* env);

Object env_get(Env* e, char* name);

// copies obj into env
void* env_set(Env* e, char* name, Object obj);
