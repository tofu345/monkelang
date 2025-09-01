#include "environment.h"
#include "object.h"

#include <stdlib.h>
#include <string.h>

Env* env_new() {
    Env* env = malloc(sizeof(Env));
    if (env == NULL) return NULL;
    env->store = ht_create();
    env->outer = NULL;
    return env;
}

Env* env_enclosed_new(Env* outer) {
    Env* env = env_new();
    if (env == NULL) return NULL;
    env->outer = outer;
    return env;
}

void env_destroy(Env* env) {
    hti iter = ht_iterator(env->store);
    while (ht_next(&iter)) {
        // TODO: GC
        object_destroy(iter.value);
    }
    ht_destroy(env->store);
    free(env);
}

Object env_get(Env* e, char* name) {
    Object* value = ht_get(e->store, name);
    if (value == NULL) {
        if (e->outer != NULL)
            return env_get(e->outer, name);
        return (Object){ o_Null };
    }
    return *value;
}

void* env_set(Env* e, char* name, Object obj) {
    Object* value = malloc(sizeof(Object));
    if (value == NULL) return NULL;
    memcpy(value, &obj, sizeof(Object));
    return (void*)ht_set(e->store, name, value);
}
