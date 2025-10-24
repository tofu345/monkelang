#include "builtin.h"
#include "allocation.h"
#include "object.h"
#include "vm.h"
#include "table.h" // provides type for "table"

#include <stdio.h>
#include <string.h>

#define ERR_NUM_ARGS(...) OBJ(o_Error, .err = error_num_args(__VA_ARGS__))

Object
builtin_exit(__attribute__ ((unused)) VM *vm, __attribute__ ((unused)) Object
        *args, int num_args) {

    if (num_args != 1) {
        return ERR_NUM_ARGS("builtin exit()", 1, num_args);
    }

    if (args[0].type != o_Integer) {
        return ERR("builtin exit() expects argument of %s got %s",
                show_object_type(o_Integer), show_object_type(args[0].type));
    }

    exit(args[0].data.integer);
}

Object
builtin_copy(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        return ERR_NUM_ARGS("builtin copy()", 1, num_args);
    }

    return object_copy(vm, args[0]);
}

Object
builtin_len(__attribute__ ((unused)) VM *vm, Object *args, int num_args) {
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

        case o_Table:
            {
                table* tbl = args[0].data.table;
                return OBJ(o_Integer, tbl->length);
            }

        default:
            return ERR("builtin len(): argument of %s not supported",
                    show_object_type(args[0].type));
    }
}

Object
builtin_first(__attribute__ ((unused)) VM *vm, Object *args, int num_args) {
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

Object
builtin_last(__attribute__ ((unused)) VM *vm, Object *args, int num_args) {
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

Object
builtin_rest(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        return ERR_NUM_ARGS("builtin rest()", 1, num_args);
    }

    switch (args[0].type) {
        case o_Array:
            {
                // create shallow copy of array.
                ObjectBuffer old_arr = *args[0].data.array;
                if (old_arr.length > 1) {
                    ObjectBuffer *new_arr =
                        create_array(vm, old_arr.data + 1, old_arr.length - 1);
                    return OBJ(o_Array, .array = new_arr);
                }

                return OBJ(o_Array, .array = create_array(vm, NULL, 0));
            }

        default:
            return ERR("builtin rest(): argument of %s not supported",
                    show_object_type(args[0].type));
    }
}

Object
builtin_push(__attribute__ ((unused)) VM *vm, Object *args, int num_args) {
    if (num_args != 2) {
        return ERR_NUM_ARGS("builtin push()", 2, num_args);
    }

    if (args[0].type != o_Array) {
        return ERR("builtin push() expects first argument to be %s got %s",
                show_object_type(o_Array), show_object_type(args[0].type));
    }

    ObjectBufferPush(args[0].data.array, args[1]);
    return args[0];
}

static void
_puts(Object obj) {
    if (obj.type == o_String) {
        printf("%s", obj.data.string->data);
    } else {
        object_fprint(obj, stdout);
    }
}

Object
builtin_puts(__attribute__ ((unused)) VM *vm, Object *args, int num_args) {
    for (int i = 0; i < num_args - 1; i++) {
        _puts(args[i]);
        printf(" ");
    }
    if (num_args >= 1)
        _puts(args[num_args - 1]);

    putc('\n', stdout);
    return NULL_OBJ;
}

#define BUILTIN(fn) {#fn, builtin_##fn}

const Builtin builtins[] = {
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

const Builtin *get_builtins() {
    return builtins;
}
