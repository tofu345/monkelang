#include "environment.h"
#include "buffer.h"
#include "object.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* allocate(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) ALLOC_FAIL();
    return ptr;
}

DEFINE_BUFFER(Frame, Frame*);
DEFINE_BUFFER(Alloc, Object*);

// mark Objects [obj] contains references to.
static void
trace_blacken_object(Object* obj) {
    switch (obj->typ) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_ReturnValue:
        case o_Error:
        case o_Function:
        case o_Closure:
            return;
        default:
            fprintf(stderr, "trace_blacken_object: type %d not handled",
                    obj->typ);
    }
}

static void
trace_mark_object(Object* obj) {
    if (obj == NULL || obj->is_marked) return;
    obj->is_marked = true;
    trace_blacken_object(obj);
}

static void
mark(Env* env) {
    for (int i = 0; i < env->frames.length; i++) {
        // check if NULL?
        ht* table = env->frames.data[i]->table;
        for (size_t idx = 0; idx < table->capacity; idx++) {
            if (table->entries[idx].key == NULL) continue;
            Object* obj = table->entries[idx].value;
            trace_mark_object(obj);
        }
    }
}

static void
allocs_remove_nulls(AllocBuffer* allocs) {
    int new_length = allocs->length;
    for (int i = 0; i < allocs->length; i++) {
        if (allocs->data[i] == NULL)
            allocs->data[i] = allocs->data[--new_length];
    }
    allocs->length = new_length;
    for (int i = new_length; i < allocs->capacity; i++)
        allocs->data[i] = NULL;
}

static void
sweep(Env* env) {
    for (int i = 0; i < env->allocs.length; i++) {
        Object* obj = env->allocs.data[i];
        if (obj == NULL) continue;
        if (obj->is_marked) {
            obj->is_marked = false;
        } else {
            if (obj->typ == o_Closure) {
                frame_destroy(obj->data.closure->frame, env);
            }
            object_destroy(obj);
            free(obj);
            env->allocs.data[i] = NULL;
        }
    }
    allocs_remove_nulls(&env->allocs);
}

void mark_and_sweep(Env* env) {
    mark(env);
    sweep(env);
}

Env* env_new() {
    Env* env = allocate(sizeof(Env));
    FrameBufferInit(&env->frames);
    AllocBufferInit(&env->allocs);

    Frame* current = allocate(sizeof(Frame));;
    current->parent = NULL;
    current->table = ht_create();
    if (current->table == NULL) ALLOC_FAIL();
    env->current = current;
    FrameBufferPush(&env->frames, current);
    return env;
}

void env_destroy(Env* env) {
    for (int i = 0; i < env->allocs.length; i++) {
        Object* obj = env->allocs.data[i];
        object_destroy(obj);
        free(obj);
    }
    free(env->allocs.data);
    for (int i = 0; i < env->frames.length; i++) {
        Frame* frame = env->frames.data[i];
        ht_destroy(frame->table);
        free(frame);
    }
    free(env->frames.data);
    free(env);
}

Frame* frame_new(Env* env) {
    Frame* new_frame = allocate(sizeof(Frame));;
    new_frame->parent = NULL;
    new_frame->table = ht_create();
    if (new_frame->table == NULL) ALLOC_FAIL();
    FrameBufferPush(&env->frames, new_frame);
    return new_frame;
}

void frame_destroy(Frame* frame, Env* env) {
    for (int i = env->frames.length; i >= 0; i--) {
        if (env->frames.data[i] == frame) {
            env->frames.data[i] = env->frames.data[--env->frames.length];
            break;
        }
    }
    ht_destroy(frame->table);
    free(frame);
}

Object* env_get(Env* env, char* name) {
    Frame* cur = env->current;
    while (cur != NULL) {
        ht* frame = cur->table;
        Object* value = ht_get(frame, name);
        if (value == NULL) {
            cur = cur->parent;
            continue;
        }
        return value;
    }
    return NULL;
}

void env_set(Env* env, char* name, Object obj) {
    Object* value = allocate(sizeof(Object));
    memcpy(value, &obj, sizeof(Object));
    AllocBufferPush(&env->allocs, value);
    if (ht_set(env->current->table, name, value) == NULL)
        ALLOC_FAIL();
}

// NOTE: could introduce a double free by inserting into [AllocBuffer]
// here and with [env_set].
// void track(Env* env, Object* ptr) {
//     AllocBufferPush(&env->allocs, ptr);
// }
