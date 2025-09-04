#pragma once

#include <stddef.h>

#include "ast.h"
#include "buffer.h"
#include "hash-table/ht.h"
#include "object.h"

void* allocate(size_t size);

// Table of variable names and ptrs to heap allocated objects.
typedef struct Frame {
    ht* table;
    struct Frame* parent;
} Frame;

struct Closure {
    Frame* frame;
    FunctionLiteral* func;
};

BUFFER(Frame, Frame*);
BUFFER(HeapObject, Object*);

typedef struct {
    FrameBuffer frames;
    Frame* current;
    HeapObjectBuffer objects; // [Objects] added to [frames] with env_set.

    // [Objects] with heap allocated members, that need to be tracked.
    HeapObjectBuffer tracked;
} Env;

Env* env_new();
void env_destroy(Env* env);

// based on boot.dev: c memory mgmt course - https://www.youtube.com/watch?v=rJrd2QMVbGM
//
// To run when: an allocation fails, or a function returns
void mark_and_sweep(Env* env);

// new [Frame] with [parent] set to NULL.
Frame* frame_new(Env* env);
void frame_destroy(Frame* frame, Env* env);

Object* env_get(Env* env, char* name);

// Copy [Object] to heap, add to [env->current] Frame, and append to
// [env->objects].
void env_set(Env* env, char* name, Object obj);

// Copy [Object] to heap, and append to [env->tracked].
//
// Meant for objects that require allocation like Closures, Arrays, Maps etc.
void env_track(Env* env, Object obj);
