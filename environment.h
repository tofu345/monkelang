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

BUFFER(Frame, ht*);
BUFFER(Alloc, Object*);

typedef struct {
    FrameBuffer frames; // key-value pairs of variable name,
                        // ptrs heap allocated objects
    AllocBuffer allocs; // tracks heap allocated [Objects]
} Env;

Env* env_new();
void env_destroy(Env* env);

void mark_and_sweep(Env* env);

// Create new [Frame] and append to [env->frames] for use with [env_set].
void frame_new(Env* env);

// Pop current frame off [FrameBuffer] and perform garbage collection.
void frame_destroy(Env* env);

Object env_get(Env* env, char* name);

// Copy [obj] to heap, add to current [Frame] (last [Frame]), and append to
// [env->allocs].
void env_set(Env* env, char* name, Object obj);

// append [ptr] to [env->allcs].
void track(Env* env, Object* ptr);
