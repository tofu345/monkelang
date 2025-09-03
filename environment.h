#pragma once

#include <stddef.h>

#include "ast.h"
#include "buffer.h"
#include "hash-table/ht.h"
#include "object.h"

void* allocate(size_t size);

// table of variable names and ptrs to heap allocated objects
typedef struct Frame {
    ht* table;
    struct Frame* parent;
} Frame;

struct Closure {
    Frame* frame;
    FunctionLiteral* func;
};

BUFFER(Frame, Frame*);
BUFFER(Alloc, Object*);

typedef struct {
    FrameBuffer frames;
    Frame* current;
    AllocBuffer allocs; // tracks allocated [Objects]
} Env;

Env* env_new();
void env_destroy(Env* env);

// garbage collector (mark and sweep).
// based on boot.dev: c memory mgmt course - https://www.youtube.com/watch?v=rJrd2QMVbGM
//
// To run when:
// - an allocation fails
// - a function returns
void mark_and_sweep(Env* env);

Frame* frame_new(Env* env);
void frame_destroy(Frame* frame, Env* env);

Object* env_get(Env* env, char* name);

// Copy [obj] to heap, add to [env->current] Frame, and append to
// [env->allocs].
void env_set(Env* env, char* name, Object obj);

// append [ptr] to [env->allcs].
void track(Env* env, Object* ptr);
