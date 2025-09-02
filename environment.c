#include "environment.h"
#include "buffer.h"
#include "object.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(Frame, ht*);
DEFINE_BUFFER(Alloc, struct HeapObject);

void* mark_and_sweep() {
}

void* allocate(ObjectType typ, size_t size, Env* env) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        // mark_and_sweep();
        // ptr = malloc(size);
        // if (ptr == NULL) return NULL;
        printf("alloc fail\n");
        exit(1);
    }
    struct HeapObject obj = { typ, ptr };
    AllocBufferPush(&env->allocs, obj);
    return ptr;
}

Env* env_new() {
    Env* env = calloc(1, sizeof(Env));
    if (env == NULL) ALLOC_FAIL();
    FrameBufferInit(&env->frames);
    AllocBufferInit(&env->allocs);

    ht* frame = ht_create();
    if (frame == NULL) ALLOC_FAIL();
    FrameBufferPush(&env->frames, frame);
    return env;
}

void* enclosed_env(Env* env) {
    ht* frame = ht_create();
    if (frame == NULL) ALLOC_FAIL();
    FrameBufferPush(&env->frames, frame);
    return env;
}

void env_destroy(Env* env) {
    fprintf(stderr, "env_destroy: not implemented\n");
    exit(1);
    // for (size_t i = 0; i < env->frames.length; i++) {
    //     ht_destroy(env->frames.data[i]);
    // }
    // for (size_t i = 0; i < env->allocs.length; i++) {
    //     object_destroy();
    // }
    // free(env);
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

void* env_set(Env* env, char* name, Object obj) {
    Object* value = malloc(sizeof(Object));
    if (value == NULL) return NULL;
    memcpy(value, &obj, sizeof(Object));
    ht* frame = env->frames.data[env->frames.length - 1];
    return (void*)ht_set(frame, name, value);
}
