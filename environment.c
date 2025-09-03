#include "environment.h"
#include "buffer.h"
#include "object.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(Frame, ht*);
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
            return;
        default:
            {
                fprintf(stderr,
                        "trace_blacken_object: type %d not handled",
                        obj->typ);
            }
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
        ht* frame = env->frames.data[i];
        for (size_t idx = 0; idx < frame->capacity; idx++) {
            if (frame->entries[idx].key == NULL) continue;
            Object* obj = frame->entries[idx].value;
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
        if (obj == NULL) {
            printf("oh no\n env->allocs contents: ");
            for (int j = 0; j < env->allocs.length; j++) {
                Object* o = env->allocs.data[j];
                if (o == NULL)
                    printf("<null>");
                else
                    printf("%s", show_object_type(o->typ));
            }
            continue;
        }
        if (obj->is_marked) {
            obj->is_marked = false;
        } else {
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
    Env* env = malloc(sizeof(Env));
    if (env == NULL) ALLOC_FAIL();
    FrameBufferInit(&env->frames);
    AllocBufferInit(&env->allocs);
    // base frame
    ht* frame = ht_create();
    if (frame == NULL) ALLOC_FAIL();
    FrameBufferPush(&env->frames, frame);
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
        ht_destroy(env->frames.data[i]);
    }
    free(env->frames.data);
    free(env);
}

void frame_new(Env* env) {
    ht* frame = ht_create();
    if (frame == NULL) ALLOC_FAIL();
    FrameBufferPush(&env->frames, frame);
}

void frame_destroy(Env* env) {
    if (env->frames.length <= 0) {
        printf("frame_destroy: empty frame buffer\n");
        exit(1);
    }
    ht* frame = FrameBufferPop(&env->frames);
    ht_destroy(frame);
    mark_and_sweep(env);
}

Object env_get(Env* env, char* name) {
    if (env->frames.length <= 0) return NULL_OBJ();
    for (int i = env->frames.length - 1; i >= 0; i--) {
        ht* frame = env->frames.data[i];
        if (frame == NULL) continue;
        Object* value = ht_get(frame, name);
        if (value == NULL) continue;
        return *value;
    }
    return NULL_OBJ();
}

void env_set(Env* env, char* name, Object obj) {
    Object* value = malloc(sizeof(Object));
    if (value == NULL) ALLOC_FAIL();
    memcpy(value, &obj, sizeof(Object));
    AllocBufferPush(&env->allocs, value);
    ht* frame = env->frames.data[env->frames.length - 1];
    if (ht_set(frame, name, value) == NULL) ALLOC_FAIL();
}

// NOTE: could introduce a double free by inserting into [AllocBuffer] here and
// with [env_set].
// void track(Env* env, Object* ptr) {
//     AllocBufferPush(&env->allocs, ptr);
// }
