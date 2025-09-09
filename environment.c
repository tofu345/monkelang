#include "environment.h"
#include "buffer.h"
#include "builtin.h"
#include "object.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(Frame, Frame*);

Object* object_new(Env* env, ObjectType typ, ObjectData data) {
    Object* obj = allocate(sizeof(Object));
    obj->typ = typ;
    obj->data = data;
    obj->is_marked = false;
    ObjectBufferPush(&env->objects, obj);
    return obj;
}

void trace_mark_object(Object* obj) {
    if (obj == NULL || obj->is_marked) return;
    obj->is_marked = true;

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
        case o_Array:
            {
                ObjectBuffer* arr = obj->data.array;
                for (int i = 0; i < arr->length; i++)
                    trace_mark_object(arr->data[i]);
                return;
            }
        default:
            fprintf(stderr, "trace_mark_object: type %d not handled",
                    obj->typ);
    }
}

static void
mark(Env* env) {
    for (int i = 0; i < env->tracking.length; i++) {
        trace_mark_object(env->tracking.data[i]);
    }
    for (int i = 0; i < env->frames.length; i++) {
        // check if NULL?
        ht* table = env->frames.data[i]->table;
        for (size_t i = 0; i < table->capacity; i++) {
            if (table->entries[i].value == NULL) continue;
            trace_mark_object(table->entries[i].value);
        }
    }
}

static void
sweep(Env* env) {
    ObjectBuffer* objs = &env->objects;
    for (int i = 0; i < objs->length; i++) {
        Object* obj = objs->data[i];
        if (obj->is_marked) {
            obj->is_marked = false;
            continue;
        }

        switch (obj->typ) {
            case o_Closure:
                frame_destroy(obj->data.closure->frame, env);
                break;
            case o_Error: continue; // ignore errors
            default: break;
        }

        object_destroy(obj);
        free(obj);

        objs->data[i] = objs->data[--objs->length];
    }
}

void mark_and_sweep(Env* env) {
    mark(env);
    sweep(env);
}

Frame* frame_new(Env* env) {
    Frame* new_frame = allocate(sizeof(Frame));
    new_frame->parent = NULL;
    new_frame->table = ht_create();
    if (new_frame->table == NULL) ALLOC_FAIL();
    FrameBufferPush(&env->frames, new_frame);
    return new_frame;
}

void frame_destroy(Frame* frame, Env* env) {
    for (int i = env->frames.length - 1; i >= 0; i--) {
        if (env->frames.data[i] == frame) {
            env->frames.data[i] = env->frames.data[--(env->frames.length)];
            break;
        }
    }
    ht_destroy(frame->table);
    free(frame);
}

Env* env_new() {
    Env* env = allocate(sizeof(Env));
    env->builtins = builtins_init();
    FrameBufferInit(&env->frames);
    env->current = frame_new(env);
    ObjectBufferInit(&env->objects);
    ObjectBufferInit(&env->tracking);
    return env;
}

void env_destroy(Env* env) {
    for (int i = 0; i < env->frames.length; i++) {
        ht_destroy(env->frames.data[i]->table);
        free(env->frames.data[i]);
    }
    free(env->frames.data);
    free(env->tracking.data);
    for (int i = 0; i < env->objects.length; i++) {
        object_destroy(env->objects.data[i]);
        free(env->objects.data[i]);
    }
    free(env->objects.data);
    ht_destroy(env->builtins);
    free(env);
}

Object* env_get(Env* env, char* name) {
    Frame* cur = env->current;
    while (cur != NULL) {
        ht* frame = cur->table;
        Object* value = ht_get(frame, name);
        if (value != NULL)
            return value;
        cur = cur->parent;
    }
    return NULL;
}

void env_set(Env* env, char* name, Object* obj) {
    if (ht_set(env->current->table, name, obj) == NULL)
        ALLOC_FAIL();
}

void track(Env *env, Object *obj) {
    ObjectBufferPush(&env->tracking, obj);
}
