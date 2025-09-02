#include "environment.h"
#include "buffer.h"
#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* mark_and_sweep() {
}

Object* allocate_object(Env* env) {
    Object* ptr = calloc(1, sizeof(Object));
    if (ptr == NULL || buffer_push(&env->allocs, (void*)ptr) == NULL) {
        // mark_and_sweep();
        // ptr = malloc(size);
        // if (ptr == NULL) return NULL;
        printf("alloc fail\n");
        exit(1);
    }
    return ptr;
}

Env* env_new() {
    Env* env = calloc(1, sizeof(Env));
    if (env == NULL) return NULL;
    if (!buffer_init(&env->frames, sizeof(ht))
            || !buffer_init(&env->allocs, sizeof(Object*))) {;
        if (env->frames.data) buffer_destroy(&env->frames);
        if (env->allocs.data) buffer_destroy(&env->allocs);
        free(env);
        return NULL;
    }

    ht* frame = ht_create();
    if (frame == NULL) {
        buffer_destroy(&env->frames);
        buffer_destroy(&env->allocs);
        free(env);
        return NULL;
    }
    buffer_push(&env->frames, (void**)frame);
    return env;
}

void* enclosed_env(Env* env) {
    ht* frame = ht_create();
    if (frame == NULL) return NULL;
    buffer_push(&env->frames, (void**)frame);
    return env;
}

void env_destroy(Env* env) {
    for (size_t i = 0; i < env->frames.len; i++) {
        ht* frame = buffer_nth(&env->frames, i);
        ht_destroy(frame);
    }
    for (size_t i = 0; i < env->allocs.len; i++) {
        Object* alloc = buffer_nth(&env->allocs, i);
        object_destroy(alloc);
    }
    free(env);
}

static Object*
_env_get(Env* env, size_t frame_n, char* name) {
    ht* frame = buffer_nth(&env->frames, frame_n);
    if (frame == NULL) return NULL;
    Object* value = ht_get(frame, name);
    if (value == NULL) return NULL;
    return value;
}

Object env_get(Env* env, char* name) {
    long i;
    for (i = env->frames.len - 1; i >= 0; i--) {
        Object* value = _env_get(env, i, name);
        if (value == NULL) continue;
        return *value;
    }
    return (Object){ o_Null };
}

void* env_set(Env* env, char* name, Object obj) {
    Object* value = malloc(sizeof(Object));
    if (value == NULL) return NULL;
    memcpy(value, &obj, sizeof(Object));
    ht* frame = buffer_nth(&env->frames, env->frames.len - 1);
    return (void*)ht_set(frame, name, value);
}
