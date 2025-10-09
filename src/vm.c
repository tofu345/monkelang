#include "vm.h"
#include "code.h"
#include "compiler.h"
#include "object.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>

DEFINE_BUFFER(HeapObject, HeapObject *);

const Object _true = OBJ(o_Boolean, .boolean = true);
const Object _false = OBJ(o_Boolean, .boolean = false);
const Object _null = (Object){};

// return ptr of size [size] and track in [vm.allocs]
static void *vm_allocate(VM *vm, ObjectType type, size_t size);
void perform_garbage_collection(VM *vm);

void vm_with(VM *vm, Bytecode *bytecode) {
    if (bytecode != NULL) {
        vm->instructions = bytecode->instructions;
        vm->constants = bytecode->constants;
    }
}

void vm_init(VM *vm, Bytecode *bytecode, Object *globals, Object *stack) {
    memset(vm, 0, sizeof(VM));

    vm_with(vm, bytecode);

    vm->nextGC = NextGC;

    if (stack == NULL) {
        stack = calloc(StackSize, sizeof(Object));
        if (stack == NULL) { die("vm_init: stack malloc"); }
    }
    vm->stack = stack;

    if (globals == NULL) {
        globals = calloc(GlobalsSize, sizeof(Object));
        if (globals == NULL) { die("vm_init: globals malloc"); }
    }
    vm->globals = globals;
}

static int
vm_push(VM *vm, Object obj) {
    if (vm->sp >= StackSize) {
        error(&vm->errors, "stack overflow");
        return -1;
    }

    vm->stack[vm->sp++] = obj;
    return 0;
}

static Object
vm_pop(VM *vm) {
    return vm->stack[--vm->sp];
}

static int
execute_binary_integer_operation(VM *vm, Opcode op, Object left, Object right) {
    long result;

    switch (op) {
        case OpAdd:
            result = left.data.integer + right.data.integer;
            break;
        case OpSub:
            result = left.data.integer - right.data.integer;
            break;
        case OpMul:
            result = left.data.integer * right.data.integer;
            break;
        case OpDiv:
            result = left.data.integer / right.data.integer;
            break;
        default:
            error(&vm->errors, "unkown integer operator: (Opcode: %d)", op);
            return -1;
    }

    return vm_push(vm, OBJ(o_Integer, .integer = result));
}

static int
execute_binary_string_operation(VM *vm, Opcode op, Object left, Object right) {
    if (op != OpAdd) {
        error(&vm->errors, "unkown string operator: (Opcode: %d)", op);
        return -1;
    }

    // copy [left] and [right] into new string;
    CharBuffer* result = vm_allocate(vm, o_String, sizeof(CharBuffer));

    result->length = left.data.string->length + right.data.string->length;
    result->capacity = power_of_2_ceil(result->length + 1);
    result->data = malloc(result->capacity * sizeof(char));
    if (result->data == NULL) { die("execute_binary_string_operation: malloc"); }
    memcpy(result->data,
            left.data.string->data,
            left.data.string->length * sizeof(char));
    memcpy(result->data + left.data.string->length,
            right.data.string->data,
            right.data.string->length * sizeof(char));
    result->data[result->length] = '\0';

    return vm_push(vm, OBJ(o_String, .string = result));
}

static int
execute_binary_operation(VM *vm, Opcode op) {
    Object right = vm_pop(vm);
    Object left = vm_pop(vm);

    if (left.type == o_Integer && right.type == o_Integer) {
        return execute_binary_integer_operation(vm, op, left, right);

    } else if (left.type == o_String && right.type == o_String) {
        return execute_binary_string_operation(vm, op, left, right);
    }

    error(&vm->errors,
            "unsupported types for binary operation: %s (Opcode: %d) %s",
            show_object_type(left.type), op, show_object_type(right.type));
    return -1;
}

static Object
native_bool_to_boolean_object(bool input) {
    return OBJ(o_Boolean, .boolean = input);
}

static int
execute_integer_comparison(VM *vm, Opcode op, Object left, Object right) {
    Object result;

    switch (op) {
        case OpEqual:
            result = native_bool_to_boolean_object(
                    left.data.integer == right.data.integer);
            break;
        case OpNotEqual:
            result = native_bool_to_boolean_object(
                    left.data.integer != right.data.integer);
            break;
        case OpGreaterThan:
            result = native_bool_to_boolean_object(
                    left.data.integer > right.data.integer);
            break;
        default:
            error(&vm->errors,
                    "unkown integer comparison operator: (Opcode: %d)", op);
            return -1;
    }
    return vm_push(vm, result);
}

static int
execute_comparison(VM *vm, Opcode op) {
    Object right = vm_pop(vm);
    Object left = vm_pop(vm);

    if (left.type == o_Integer && right.type == o_Integer) {
        return execute_integer_comparison(vm, op, left, right);
    }

    bool eq = object_eq(left, right);

    switch (op) {
        case OpEqual:
            return vm_push(vm, native_bool_to_boolean_object(eq));
        case OpNotEqual:
            return vm_push(vm, native_bool_to_boolean_object(!eq));
        default:
            error(&vm->errors, "unkown comparison operator: (Opcode: %d) (%s %s)",
                    op, show_object_type(left.type), show_object_type(right.type));
            return -1;
    }
}

static int
execute_bang_operator(VM *vm) {
    Object operand = vm_pop(vm);
    switch (operand.type) {
        case o_Boolean:
            operand.data.boolean = !operand.data.boolean;
            return vm_push(vm, operand);
        case o_Null:
            return vm_push(vm, _true);
        default:
            return vm_push(vm, _false);
    }
}

static int
execute_minus_operator(VM *vm) {
    Object operand = vm_pop(vm);
    if (operand.type != o_Integer) {
        error(&vm->errors, "unsupported type for negation: %s",
                show_object_type(operand.type));
        return -1;
    }
    operand.data.integer = -operand.data.integer;
    return vm_push(vm, operand);
}

static bool
is_truthy(Object obj) {
    switch (obj.type) {
        case o_Boolean:
            return obj.data.boolean;
        case o_Null:
            return false;
        case o_Array:
            return obj.data.array->length > 0;
        default:
            return true;
    }
}

static Object
create_string_object(VM *vm, Constant str) {
    size_t len = strlen(str.data.string);
    CharBuffer* str_obj = vm_allocate(vm, o_String, sizeof(CharBuffer));
    if (len > 0) {
        str_obj->capacity = power_of_2_ceil(len + 1);
        str_obj->data = malloc(str_obj->capacity * sizeof(char));
        if (str_obj->data == NULL) {
            die("create_string_object: malloc");
        }

        memcpy(str_obj->data, str.data.string, len * sizeof(char));
        str_obj->data[len] = '\0';
        str_obj->length = len;
    } else {
        memset(str_obj, 0, sizeof(CharBuffer));
    }
    return OBJ(o_String, .string = str_obj);
}

static Object
constant_to_object(VM *vm, Constant obj_const) {
    switch (obj_const.type) {
        case c_Integer:
            return OBJ(o_Integer, .integer = obj_const.data.integer);
        case c_Float:
            return OBJ(o_Float, .floating = obj_const.data.integer);
        case c_String:
            return create_string_object(vm, obj_const);
        default:
            die("constant_to_object: type %d not handled", obj_const.type);
            return (Object){};
    }
}

static Object
build_array(VM *vm, int start_index, int end_index) {
    int length = end_index - start_index;
    ObjectBuffer *elems = vm_allocate(vm, o_Array, sizeof(ObjectBuffer));

    if (length <= 0) {
        memset(elems, 0, sizeof(ObjectBuffer));
    } else {
        elems->length = length;
        elems->capacity = power_of_2_ceil(length);
        int size = elems->capacity * sizeof(Object);
        elems->data = malloc(size);
        if (elems->data == NULL) { die("build_array: malloc"); }

        // NOTE: must manually decrement
        vm->nextGC -= size;

        for (int i = start_index; i < end_index; i++) {
            elems->data[i - start_index] = vm->stack[i];
        }
    }

    return OBJ(o_Array, .array = elems);
}

int vm_run(VM *vm) {
    Opcode op;
    int err, pos, num;
    Object obj;

    for (int ip = 0; ip < vm->instructions.length; ip++) {
        // see [vm_allocate()]
        if (vm->nextGC <= 0) {
            perform_garbage_collection(vm);
            vm->nextGC = NextGC;
        }

        op = vm->instructions.data[ip];
        switch (op) {
            case OpConstant:
                // constants index
                num = read_big_endian_uint16(vm->instructions.data + ip + 1);
                ip += 2;

                obj = constant_to_object(vm, vm->constants.data[num]);
                err = vm_push(vm, obj);
                if (err == -1) { return -1; };
                break;

            case OpAdd:
            case OpSub:
            case OpMul:
            case OpDiv:
                err = execute_binary_operation(vm, op);
                if (err == -1) { return -1; };
                break;

            case OpPop:
                vm_pop(vm);
                break;

            case OpTrue:
                err = vm_push(vm, _true);
                if (err == -1) { return -1; };
                break;
            case OpFalse:
                err = vm_push(vm, _false);
                if (err == -1) { return -1; };
                break;

            case OpEqual:
            case OpNotEqual:
            case OpGreaterThan:
                err = execute_comparison(vm, op);
                if (err == -1) { return -1; };
                break;

            case OpBang:
                err = execute_bang_operator(vm);
                if (err == -1) { return -1; };
                break;
            case OpMinus:
                err = execute_minus_operator(vm);
                if (err == -1) { return -1; };
                break;

            case OpJump:
                pos = read_big_endian_uint16(vm->instructions.data + ip + 1);
                ip = pos - 1;
                break;
            case OpJumpNotTruthy:
                pos = read_big_endian_uint16(vm->instructions.data + ip + 1);
                ip += 2;

                obj = vm_pop(vm);
                if (!is_truthy(obj)) {
                    ip = pos - 1;
                }
                break;

            case OpNull:
                err = vm_push(vm, _null);
                if (err == -1) { return -1; };
                break;

            case OpSetGlobal:
                // globals index
                num = read_big_endian_uint16(vm->instructions.data + ip + 1);
                ip += 2;

                vm->globals[num] = vm_pop(vm);
                break;
            case OpGetGlobal:
                // globals index
                num = read_big_endian_uint16(vm->instructions.data + ip + 1);
                ip += 2;

                err = vm_push(vm, vm->globals[num]);
                if (err == -1) { return -1; };
                break;

            case OpArray:
                // number of array elements
                num = read_big_endian_uint16(vm->instructions.data + ip + 1);
                ip += 2;

                obj = build_array(vm, vm->sp - num, vm->sp);
                vm->sp -= num;

                err = vm_push(vm, obj);
                if (err == -1) { return -1; };
                break;

            default:
                error(&vm->errors, "unknown opcode %d", op);
                return -1;
        }
    }
    return 0;
}

Object stack_top(VM *vm) {
    if (vm->sp == 0) {
        return (Object){};
    }
    return vm->stack[vm->sp - 1];
}

Object vm_last_popped(VM *vm) {
    return vm->stack[vm->sp];
}

struct _heap_object {
    HeapObject obj;
    void *ptr;
};

static void *
vm_allocate(VM *vm, ObjectType type, size_t size) {
    // GC is triggered before start of main loop to avoid use after frees of
    // intermediate objects.
    vm->nextGC -= size;

    // prepend [HeapObject] to allocated memory.
    HeapObject *h_obj = malloc(sizeof(HeapObject) + size);
    if (h_obj == NULL) { die("vm_allocate: malloc"); }

    // get pointer to allocated memory after [HeapObject].
    void *obj = &((struct _heap_object *)h_obj)->ptr;

    *h_obj = (HeapObject){
        .is_marked = false,
        .ptr = obj,
        .type = type,
    };

    HeapObjectBufferPush(&vm->allocs, h_obj);
    return obj;
}

void heap_object_free(HeapObject *h_obj) {
    switch (h_obj->type) {
        case o_String:
            free(((CharBuffer *)h_obj->ptr)->data);
            break;
        case o_Array:
            free(((ObjectBuffer *)h_obj->ptr)->data);
            break;
        // case o_Error:
        //     break;
        // case o_Hash:
        //     break;
        default:
            printf("heap_object_destroy: %s (%d) typ not handled\n",
                    show_object_type(h_obj->type), h_obj->type);
            exit(1);
            return;
    }
    free(h_obj);
}

void vm_free(VM *vm) {
    for (int i = 0; i < vm->errors.length; i++) {
        free(vm->errors.data[i]);
    }
    free(vm->errors.data);
    for (int i = 0; i < vm->allocs.length; i++) {
        heap_object_free(vm->allocs.data[i]);
    }
    free(vm->allocs.data);
    free(vm->stack);
    free(vm->globals);
}

static void
mark_heap_object(Object obj) {
    assert(obj.type >= o_String);
    // access HeapObject prepended to ptr with [vm_allocate()]
    HeapObject *h_obj = ((HeapObject *)obj.data.ptr) - 1;
    h_obj->is_marked = true;
}

static void
trace_mark_object(Object obj) {
    switch (obj.type) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
            return;
        case o_Error:
        case o_String:
            mark_heap_object(obj);
            return;
        case o_Array:
            {
                mark_heap_object(obj);
                ObjectBuffer* arr = obj.data.array;
                for (int i = 0; i < arr->length; i++)
                    trace_mark_object(arr->data[i]);
                return;
            }
        // case o_Hash:
        //     {
        //         hti it = ht_iterator(obj.data.hash);
        //         while (ht_next(&it)) {
        //             // trace_mark_object(it.current->value);
        //         }
        //         return;
        //     }
        default:
            die("trace_mark_object: type %s (%d) not handled",
                    show_object_type(obj.type), obj.type);
    }
}

void perform_garbage_collection(VM *vm) {
    printf("perform_garbage_collection\n");
    int i, sp = vm->sp,
        allocs_length = vm->allocs.length;

    // mark
    Object *stack = vm->stack;
    for (i = sp; i >= 0; i--) {
        if (stack[i].type >= o_String) {
            trace_mark_object(stack[i]);
        }
    }

    // sweep
    HeapObject *heap_obj;
    HeapObject **allocs = vm->allocs.data;
    int end = allocs_length;
    for (i = 0; i < allocs_length; i++) {
        heap_obj = allocs[i];
        if (heap_obj->is_marked) {
            heap_obj->is_marked = false;
            continue;
        }
        heap_object_free(heap_obj);
        allocs[i] = allocs[--end];
    }
    vm->allocs.length = end;
}
