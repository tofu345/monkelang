#include "builtin.h"
#include "object.h"
#include "utils.h"
#include "evaluator.h"

Object* builtin_exit(Env* env, ObjectBuffer* args) {
    if (args->length > 0)
        return new_error(env, "builtin exit() takes 0 arguments got %d",
                args->length);

    exit(0);
}

Object* builtin_copy(Env* env, ObjectBuffer* args) {
    if (args->length != 1)
        return new_error(env, "builtin copy() takes 1 argument got %d",
                args->length);

    return object_copy(env, args->data[0]);
}

Object* builtin_len([[maybe_unused]] Env* env, ObjectBuffer* args) {
    if (args->length != 1)
        return new_error(env, "builtin len() takes 1 argument got %d",
                args->length);

    switch (args->data[0]->typ) {
        case o_String:
            {
                CharBuffer* str = args->data[0]->data.string;
                return OBJ(o_Integer, str->length);
            }
        case o_Array:
            {
                ObjectBuffer* arr = args->data[0]->data.array;
                return OBJ(o_Integer, arr->length);
            }
        default:
            return new_error(env, "builtin len(): argument of '%s' not supported",
                    show_object_type(args->data[0]->typ));
    }
}

Object* builtin_first([[maybe_unused]] Env* env, ObjectBuffer* args) {
    if (args->length != 1)
        return new_error(env, "builtin first() takes 1 argument got %d",
                args->length);

    switch (args->data[0]->typ) {
        // TODO? char datatype?
        case o_Array:
            {
                ObjectBuffer* arr = args->data[0]->data.array;
                if (arr->length > 0)
                    return arr->data[0];
                return NULL;
            }
        default:
            return new_error(env, "builtin first(): argument of '%s' not supported",
                    show_object_type(args->data[0]->typ));
    }
}

Object* builtin_last([[maybe_unused]] Env* env, ObjectBuffer* args) {
    if (args->length != 1)
        return new_error(env, "builtin last() takes 1 argument got %d",
                args->length);

    switch (args->data[0]->typ) {
        case o_Array:
            {
                ObjectBuffer* arr = args->data[0]->data.array;
                if (arr->length > 0)
                    return arr->data[arr->length - 1];
                return NULL;
            }
        default:
            return new_error(env, "builtin last(): argument of '%s' not supported",
                    show_object_type(args->data[0]->typ));
    }
}

Object* builtin_rest([[maybe_unused]] Env* env, ObjectBuffer* args) {
    if (args->length != 1)
        return new_error(env, "builtin rest() takes 1 argument got %d",
                args->length);

    switch (args->data[0]->typ) {
        case o_Array:
            {
                ObjectBuffer* arr = args->data[0]->data.array;
                Object* new_obj = object_copy(env, args->data[0]);
                ObjectBuffer* new_arr = new_obj->data.array;
                int len = arr->length;
                if (len >= 1) {
                    for (int i = 0; i < len - 1; i++) {
                        new_arr->data[i] = new_arr->data[i + 1];
                    }
                    new_arr->length--;
                }
                return new_obj;
            }
        default:
            return new_error(env, "builtin rest(): argument of '%s' not supported",
                    show_object_type(args->data[0]->typ));
    }
}

Object* builtin_push(Env* env, ObjectBuffer* args) {
    if (args->length != 2)
        return new_error(env, "builtin push() takes 2 arguments got %d",
                args->length);

    if (args->data[0]->typ != o_Array)
        return new_error(env,
                "builtin push() expects first argument to be 'Array' got %s",
                show_object_type(args->data[0]->typ));

    ObjectBufferPush(args->data[0]->data.array, args->data[1]);
    return args->data[0];
}

Object* builtin_puts([[maybe_unused]] Env* env, ObjectBuffer* args) {
    for (int i = 0; i < args->length; i++) {
        object_fprint(args->data[i], stdout);
        puts("");
    }
    return NULL;
}

#define BUILTIN(fn) \
    {#fn, (Object){ \
        o_BuiltinFunction, false, {.builtin = (Builtin*)builtin_##fn} \
    }}

struct {
    const char* name;
    Object obj;
} const builtins[] = {
    BUILTIN(exit),
    BUILTIN(copy),
    BUILTIN(len),
    BUILTIN(first),
    BUILTIN(last),
    BUILTIN(rest),
    BUILTIN(push),
    BUILTIN(puts),
};

ht* builtins_init() {
    ht* tbl = ht_create();
    if (tbl == NULL) ALLOC_FAIL();
    int len = sizeof(builtins) / sizeof(builtins[0]);
    for (int i = 0; i < len; i++) {
        if (ht_set(tbl, builtins[i].name, (void *)&builtins[i].obj) == NULL)
            ALLOC_FAIL();
    }
    return tbl;
}
