#include "builtin.h"
#include "object.h"
#include "vm.h"
#include "table.h" // provides type for "table"

#include <string.h>

#define ERR_NUM_ARGS(...) OBJ(o_Error, .err = error_num_args(__VA_ARGS__))

Object builtin_exit([[maybe_unused]] VM *vm, [[maybe_unused]] Object *args, int num_args) {
    if (num_args != 0) {
        return ERR_NUM_ARGS("builtin exit()", 0, num_args);
    }

    exit(0);
}

Object builtin_copy(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        return ERR_NUM_ARGS("builtin copy()", 1, num_args);
    }

    return object_copy(vm, args[0]);
}

Object builtin_len([[maybe_unused]] VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        return ERR_NUM_ARGS("builtin len()", 1, num_args);
    }

    switch (args[0].type) {
        case o_String:
            {
                CharBuffer* str = args[0].data.string;
                return OBJ(o_Integer, str->length);
            }

        case o_Array:
            {
                ObjectBuffer* arr = args[0].data.array;
                return OBJ(o_Integer, arr->length);
            }

        case o_Hash:
            {
                table* tbl = args[0].data.hash;
                return OBJ(o_Integer, tbl->length);
            }

        default:
            return ERR("builtin len(): argument of %s not supported",
                    show_object_type(args[0].type));
    }
}

Object builtin_first([[maybe_unused]] VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        return ERR_NUM_ARGS("builtin first()", 1, num_args);
    }

    switch (args[0].type) {
        case o_Array:
            {
                ObjectBuffer* arr = args[0].data.array;
                if (arr->length > 0) {
                    return arr->data[0];
                }
                return NULL_OBJ;
            }
            break;

        default:
            return ERR("builtin first(): argument of %s not supported",
                    show_object_type(args[0].type));
    }

}

Object builtin_last([[maybe_unused]] VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        return ERR_NUM_ARGS("builtin last()", 1, num_args);
    }

    switch (args[0].type) {
        case o_Array:
            {
                ObjectBuffer* arr = args[0].data.array;
                if (arr->length > 0) {
                    return arr->data[arr->length - 1];
                }
                return NULL_OBJ;
            }

        default:
            return ERR("builtin last(): argument of %s not supported",
                    show_object_type(args[0].type));
    }
}

Object builtin_rest(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        return ERR_NUM_ARGS("builtin rest()", 1, num_args);
    }

    switch (args[0].type) {
        case o_Array:
            {
                // create shallow copy of array.
                ObjectBuffer old_arr = *args[0].data.array;
                int new_length = old_arr.length - 1;
                ObjectBuffer *new_arr;
                if (old_arr.length > 1) {
                    Object *new_buf = allocate(vm, old_arr.capacity * sizeof(Object));

                    // copy from second element.
                    memcpy(new_buf, old_arr.data + 1, new_length * sizeof(Object));

                    new_arr = COMPOUND_OBJ(o_Array, ObjectBuffer, {
                        .data = new_buf,
                        .length = new_length,
                        .capacity = old_arr.capacity
                    });
                    return OBJ(o_Array, .array = new_arr);

                } else {
                    return NULL_OBJ;
                }
            }

        default:
            return ERR("builtin rest(): argument of %s not supported",
                    show_object_type(args[0].type));
    }
}

Object builtin_push([[maybe_unused]] VM *vm, Object *args, int num_args) {
    if (num_args != 2) {
        return ERR_NUM_ARGS("builtin push()", 2, num_args);
    }

    if (args[0].type != o_Array) {
        return ERR("builtin push() expects first argument to be Array got %s",
                show_object_type(args[0].type));
    }

    ObjectBufferPush(args[0].data.array, args[1]);
    return args[0];
}

Object builtin_puts([[maybe_unused]] VM *vm, Object *args, int num_args) {
    for (int i = 0; i < num_args - 1; i++) {
        object_fprint(args[i], stdout);
        printf(" ");
    }
    if (num_args >= 1)
        object_fprint(args[num_args - 1], stdout);

    putc('\n', stdout);
    return NULL_OBJ;
}

#define BUILTIN(fn) {#fn, builtin_##fn}

const Builtins builtins[] = {
    BUILTIN(len),
    BUILTIN(puts),
    BUILTIN(first),
    BUILTIN(last),
    BUILTIN(rest),
    BUILTIN(push),
    BUILTIN(exit),
    BUILTIN(copy),
    { NULL, NULL },
};

const Builtins *get_builtins() {
    return builtins;
}
