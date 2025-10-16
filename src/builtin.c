#include "builtin.h"
#include "object.h"
#include "utils.h"
#include "vm.h"
#include "table.h"

#include <string.h>

int builtin_exit(VM *vm, [[maybe_unused]] Object *args, int num_args) {
    if (num_args != 0) {
        error_num_args(&vm->errors, "builtin exit()", 0, num_args);
        return -1;
    }

    exit(0);
}

int builtin_copy(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        error_num_args(&vm->errors, "builtin copy()", 1, num_args);
        return -1;
    }

    return vm_push(vm, object_copy(vm, args[0]));
}

int builtin_len(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        error_num_args(&vm->errors, "builtin len()", 1, num_args);
        return -1;
    }

    Object result;
    switch (args[0].type) {
        case o_String:
            {
                CharBuffer* str = args[0].data.string;
                result = OBJ(o_Integer, str->length);
                break;
            }

        case o_Array:
            {
                ObjectBuffer* arr = args[0].data.array;
                result = OBJ(o_Integer, arr->length);
                break;
            }

        case o_Hash:
            {
                table* tbl = args[0].data.hash;
                result = OBJ(o_Integer, tbl->length);
                break;
            }

        default:
            error(&vm->errors, "builtin len(): argument of %s not supported",
                    show_object_type(args[0].type));
            return -1;
    }

    return vm_push(vm, result);
}

int builtin_first(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        error_num_args(&vm->errors, "builtin first()", 1, num_args);
        return -1;
    }

    Object result;
    switch (args[0].type) {
        case o_Array:
            {
                ObjectBuffer* arr = args[0].data.array;
                if (arr->length > 0) {
                    result = arr->data[0];
                } else {
                    result = NULL_OBJ;
                }
            }
            break;

        default:
            error(&vm->errors, "builtin first(): argument of %s not supported",
                    show_object_type(args[0].type));
            return -1;
    }

    return vm_push(vm, result);
}

int builtin_last(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        error_num_args(&vm->errors, "builtin last()", 1, num_args);
        return -1;
    }

    Object result;
    switch (args[0].type) {
        case o_Array:
            {
                ObjectBuffer* arr = args[0].data.array;
                if (arr->length > 0) {
                    result = arr->data[arr->length - 1];
                } else {
                    result = NULL_OBJ;
                }
                break;
            }

        default:
            error(&vm->errors, "builtin last(): argument of %s not supported",
                    show_object_type(args[0].type));
            return -1;
    }

    return vm_push(vm, result);
}

int builtin_rest(VM *vm, Object *args, int num_args) {
    if (num_args != 1) {
        error_num_args(&vm->errors, "builtin rest()", 1, num_args);
        return -1;
    }

    switch (args[0].type) {
        case o_Array:
            {
                ObjectBuffer *old = args[0].data.array;
                Object new_obj;
                if (old->length > 1) {
                    // copy from first element
                    old->data++;
                    old->length--;

                    new_obj = object_copy(vm, args[0]);

                    // revert
                    old->data--;
                    old->length++;

                } else {
                    new_obj = NULL_OBJ;
                }

                return vm_push(vm, new_obj);
            }

        default:
            error(&vm->errors, "builtin rest(): argument of %s not supported",
                    show_object_type(args[0].type));
            return -1;
    }
}

int builtin_push(VM *vm, Object *args, int num_args) {
    if (num_args != 2) {
        error_num_args(&vm->errors, "builtin push()", 2, num_args);
        return -1;
    }

    if (args[0].type != o_Array) {
        error(&vm->errors,
                "builtin push() expects first argument to be Array got %s",
                show_object_type(args[0].type));
        return -1;
    }

    ObjectBufferPush(args[0].data.array, args[1]);
    return vm_push(vm, args[0]);
}

int builtin_puts([[maybe_unused]] VM *vm, Object *args, int num_args) {
    for (int i = 0; i < num_args - 1; i++) {
        object_fprint(args[i], stdout);
        printf(" ");
    }
    if (num_args >= 1)
        object_fprint(args[num_args - 1], stdout);

    putc('\n', stdout);
    return vm_push(vm, NULL_OBJ);
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
