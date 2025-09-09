#pragma once

#include <stddef.h>

#include "ast.h"
#include "buffer.h"
#include "hash-table/ht.h"
#include "object.h"

#define OBJ(t, d) object_new(env, t, (ObjectData){d})

Object* object_new(Env* env, ObjectType typ, ObjectData data);

typedef struct Frame {
    // Table of variable names and ptrs to heap allocated objects.
    ht* table;
    struct Frame* parent;
} Frame;

struct Closure {
    Frame* frame;
    FunctionLiteral* func;
};

BUFFER(Frame, Frame*);

struct Env {
    FrameBuffer frames;
    Frame* current;
    ObjectBuffer objects;
    ObjectBuffer tracking; // [Objects] that must stay in scope
    ht* builtins;
};

Env* env_new();
void env_destroy(Env* env);

// new [Frame] with [parent] set to NULL.
Frame* frame_new(Env* env);
void frame_destroy(Frame* frame, Env* env);

// based on boot.dev: c memory mgmt course - https://www.youtube.com/watch?v=rJrd2QMVbGM
//
// To run when: an allocation fails, or a function returns
void mark_and_sweep(Env* env);
void trace_mark_object(Object* obj);

Object* env_get(Env* env, char* name);

// add to [env->current] table, and append to [env->objects].
void env_set(Env* env, char* name, Object* obj);

// add to [env->tracking]
void track(Env* env, Object* obj);
