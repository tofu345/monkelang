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
DEFINE_BUFFER(HeapObject, Object*);

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

void print_heap_object_buffer(HeapObjectBuffer* buf) {
    for (int i = 0; i < buf->capacity; i++) {
        printf("%c", buf->data[i] != NULL ? 'X' : 'O');
    }
    printf("\n");
}

static void
sweep_heap_objects(HeapObjectBuffer* buf, Env* env) {
    // printf("before: \n");
    // print_heap_object_buffer(buf);

    for (int i = 0; i < buf->length; i++) {
        Object* obj = buf->data[i];
        if (obj->is_marked) {
            obj->is_marked = false;
            continue;
        }
        if (obj->typ == o_Closure) {
            frame_destroy(obj->data.closure->frame, env);
        }
        object_destroy(obj);
        free(obj);
        buf->data[i] = buf->data[--buf->length];
        buf->data[buf->length] = NULL;
    }

    // printf("after: \n");
    // print_heap_object_buffer(buf);
}

static void
sweep(Env* env) {
    sweep_heap_objects(&env->objects, env);
    sweep_heap_objects(&env->tracked, env);
}

void mark_and_sweep(Env* env) {
    mark(env);
    sweep(env);
}

Env* env_new() {
    Env* env = allocate(sizeof(Env));
    FrameBufferInit(&env->frames);
    HeapObjectBufferInit(&env->objects);
    HeapObjectBufferInit(&env->tracked);
    Frame* current = allocate(sizeof(Frame));;
    current->parent = NULL;
    current->table = ht_create();
    if (current->table == NULL) ALLOC_FAIL();
    env->current = current;
    FrameBufferPush(&env->frames, current);
    return env;
}

void destroy_heap_object(HeapObjectBuffer* buf) {
    for (int i = 0; i < buf->length; i++) {
        Object* obj = buf->data[i];
        object_destroy(obj);
        free(obj);
    }
    free(buf->data);
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

void env_destroy(Env* env) {
    destroy_heap_object(&env->objects);
    destroy_heap_object(&env->tracked);
    for (int i = 0; i < env->frames.length; i++)
        frame_destroy(env->frames.data[i], env);
    free(env->frames.data);
    free(env);
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

// search for [obj.data] in [env->tracked], remove and return if found.
//
// could always be found at the last index, but not sure
static Object*
search_tracked_for(Object obj, Env* env) {
    // printf("search_tracked_for: tracked length = %d\n", env->tracked.length);
    for (int i = env->tracked.length; i >= 0; i--) {
        Object* tracked_obj = env->tracked.data[i];
        if (tracked_obj == NULL) continue;
        if (memcmp(&tracked_obj->data, &obj.data, sizeof(ObjectData)) == 0) {
            // printf("tracked object found at %d\n", i);
            env->tracked.data[i] = env->tracked.data[--env->tracked.length];
            env->tracked.data[env->tracked.length] = NULL;
            return tracked_obj;
        }
    }
    return NULL;
}

void env_set(Env* env, char* name, Object obj) {
    Object* value = NULL;
    switch (obj.typ) {
        case o_Closure:
            value = search_tracked_for(obj, env);
            // printf("env_set: closure %s\n", value == NULL ? "null" : "not null");
            break;
        default: break;
    }
    if (value == NULL) {
        value = allocate(sizeof(Object));
        memcpy(value, &obj, sizeof(Object));
    }
    HeapObjectBufferPush(&env->objects, value);
    if (ht_set(env->current->table, name, value) == NULL)
        ALLOC_FAIL();
}

void env_track(Env* env, Object obj) {
    Object* ptr = allocate(sizeof(Object));
    memcpy(ptr, &obj, sizeof(Object));
    HeapObjectBufferPush(&env->tracked, ptr);
}
