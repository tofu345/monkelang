#include "vm.h"
#include "builtin.h"
#include "code.h"
#include "compiler.h"
#include "constants.h"
#include "frame.h"
#include "object.h"
#include "table.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>

const Object _true = OBJ(o_Boolean, .boolean = true);
const Object _false = OBJ(o_Boolean, .boolean = false);
const Object _null = (Object){};

DEFINE_BUFFER(Alloc, Alloc *);

// track [Alloc]ation of [size] and garbage collect if necessary.
static void *vm_allocate(VM *vm, ObjectType type, size_t size);

// allocate [size] but do *not* garbage collect.
// - necessary to avoid triggering garbage collection on objects not on the
// [vm.stack] but still in use. see [execute_binary_string_operation]
static void *allocate(VM *vm, size_t size);

void vm_init(VM *vm, Object *stack, Object *globals, Frame *frames) {
    memset(vm, 0, sizeof(VM));

    vm->bytesTillGC = NextGC;

    if (stack == NULL) {
        stack = calloc(StackSize, sizeof(Object));
        if (stack == NULL) { die("vm stack create"); }
    }
    vm->stack = stack;

    if (globals == NULL) {
        globals = calloc(GlobalsSize, sizeof(Object));
        if (globals == NULL) { die("vm globals create"); }
    }
    vm->globals = globals;

    if (frames == NULL) {
        frames = calloc(MaxFraxes, sizeof(Frame));
        if (frames == NULL) { die("vm frames create"); }
    }
    vm->frames = frames;
}

void vm_with(VM *vm, Bytecode bytecode) {
    CompiledFunction main_fn = {
        .instructions = *bytecode.instructions
    };
    frame_init(vm->frames, main_fn, 0);
    vm->frames_index = 1;

    vm->constants = *bytecode.constants;
}

Object object_copy(VM* vm, Object obj) {
    switch (obj.type) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_CompiledFunction:
        case o_BuiltinFunction:
            return obj;
        case o_String:
            {
                CharBuffer old_str = *obj.data.string;
                CharBuffer* new_str;
                if (old_str.length > 1) {
                    // copy string contents
                    char *new_buf = allocate(vm, old_str.capacity * sizeof(char));
                    memcpy(new_buf, old_str.data, old_str.length * sizeof(char));
                    new_buf[old_str.length] = '\0';

                    // copy allocated [Object]
                    new_str = vm_allocate(vm, o_String, sizeof(CharBuffer));
                    *new_str = (CharBuffer) {
                        .data = new_buf,
                            .length = old_str.length,
                            .capacity = old_str.capacity
                    };
                    return OBJ(o_String, .string = new_str);
                } else {
                    new_str = vm_allocate(vm, o_String, sizeof(CharBuffer));
                    memset(new_str, 0, sizeof(CharBuffer));
                }
                return OBJ(o_String, .string = new_str);
            }
        case o_Array:
            {
                ObjectBuffer old_arr = *obj.data.array;
                ObjectBuffer* new_arr;
                if (old_arr.length > 1) {
                    // copy array and contents individually
                    Object *new_buf = allocate(vm, old_arr.capacity * sizeof(Object));
                    new_arr = vm_allocate(vm, o_Array, sizeof(ObjectBuffer));
                    *new_arr = (ObjectBuffer) {
                        .data = new_buf,
                            .length = old_arr.length,
                            .capacity = old_arr.capacity
                    };
                    for (int i = 0; i < old_arr.length; i++) {
                        new_buf[i] = object_copy(vm, old_arr.data[i]);
                    }
                } else {
                    new_arr = vm_allocate(vm, o_Array, sizeof(ObjectBuffer));
                    memset(new_arr, 0, sizeof(ObjectBuffer));
                }
                return OBJ(o_Array, .array = new_arr);
            }
        default:
            die("object_copy: object type not handled %d\n", obj.type);
            return NULL_OBJ;
    }
}

Frame *pop_frame(VM *vm) {
    vm->frames_index--;
    return vm->frames + vm->frames_index - 1;
}

Frame *new_frame(VM *vm) {
    vm->frames_index++;
    return vm->frames + vm->frames_index - 1;
}

int vm_push(VM *vm, Object obj) {
    if (vm->sp >= StackSize) {
        error(&vm->errors, "stack overflow");
        return -1;
    }

    vm->stack[vm->sp++] = obj;
    return 0;
}

Object vm_pop(VM *vm) {
    return vm->stack[--vm->sp];
}

static int
execute_binary_float_operation(VM *vm, Opcode op, Object left, Object right) {
    double result;

    switch (op) {
        case OpAdd:
            result = left.data.floating + right.data.floating;
            break;
        case OpSub:
            result = left.data.floating - right.data.floating;
            break;
        case OpMul:
            result = left.data.floating * right.data.floating;
            break;
        case OpDiv:
            result = left.data.floating / right.data.floating;
            break;
        default:
            error(&vm->errors, "unkown float operator: (Opcode: %d)", op);
            return -1;
    }

    return vm_push(vm, OBJ(o_Float, .floating = result));
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

    // copy [left] and [right] into new string
    // NOTE: must occur before [vm_allocate], if not, potential use after free.
    int length = left.data.string->length + right.data.string->length,
        capacity = power_of_2_ceil(length + 1);
    char *result = allocate(vm, capacity * sizeof(char));

    memcpy(result,
            left.data.string->data,
            left.data.string->length * sizeof(char));
    memcpy(result + left.data.string->length,
            right.data.string->data,
            right.data.string->length * sizeof(char));
    result[length] = '\0';

    CharBuffer* obj_data = vm_allocate(vm, o_String, sizeof(CharBuffer));
    *obj_data = (CharBuffer){
        .data = result,
        .length = length,
        .capacity = capacity
    };

    return vm_push(vm, OBJ(o_String, .string = obj_data));
}

static int
execute_binary_operation(VM *vm, Opcode op) {
    Object right = vm_pop(vm);
    Object left = vm_pop(vm);

    if (left.type == o_Integer && right.type == o_Integer) {
        return execute_binary_integer_operation(vm, op, left, right);

    } else if (left.type == o_Float && right.type == o_Float) {
        return execute_binary_float_operation(vm, op, left, right);

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

static int
execute_array_index(VM *vm, Object array, Object index) {
    ObjectBuffer *arr = array.data.array;
    int i = index.data.integer,
        max = arr->length - 1;
    if (i < 0 || i > max) {
        return vm_push(vm, _null);
    }

    return vm_push(vm, arr->data[i]);
}

static int
execute_hash_index(VM *vm, Object hash, Object index) {
    table *tbl = hash.data.hash;
    if (!hashable(index)) {
        error(&vm->errors, "unusable as hash key: %s",
                show_object_type(index.type));
        return -1;
    }

    return vm_push(vm, table_get(tbl, index));
}

static int
execute_index_expression(VM *vm) {
    Object index = vm_pop(vm);
    Object left = vm_pop(vm);

    if (left.type == o_Array && index.type == o_Integer) {
        return execute_array_index(vm, left, index);

    } else if (left.type == o_Hash) {
        return execute_hash_index(vm, left, index);

    } else {
        error(&vm->errors, "index operator not supported: %s",
                show_object_type(left.type));
        return -1;
    }
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

static int
vm_push_constant(VM *vm, Constant c) {
    switch (c.type) {
        case c_Integer:
            return vm_push(vm, OBJ(o_Integer, .integer = c.data.integer));

        case c_Float:
            return vm_push(vm, OBJ(o_Float, .floating = c.data.floating));

        case c_String:
            {
                StringConstant str = *c.data.string;
                size_t capacity = 0;
                char *buf = NULL;
                if (str.length > 0) {
                    capacity = power_of_2_ceil(str.length + 1);
                    buf = allocate(vm, capacity * sizeof(char));
                    memcpy(buf, str.data, str.length * sizeof(char));
                    buf[str.length] = '\0';
                }

                CharBuffer* str_buf = vm_allocate(vm, o_String, sizeof(CharBuffer));
                *str_buf = (CharBuffer){
                    .data = buf,
                    .length = str.length,
                    .capacity = capacity
                };
                return vm_push(vm, OBJ(o_String, .string = str_buf));
            }

        case c_Function:
            return vm_push(vm, OBJ(o_CompiledFunction, .func = c.data.function));

        default:
            die("vm_push_constant: type %d not handled", c.type);
            return -1;
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
        elems->data = allocate(vm, size);

        for (int i = start_index; i < end_index; i++) {
            elems->data[i - start_index] = vm->stack[i];
        }
    }

    return OBJ(o_Array, .array = elems);
}

static Object
build_hash(VM *vm, int start_index, int end_index) {
    table *tbl = vm_allocate(vm, o_Hash, sizeof(table));
    if (table_init(tbl) == NULL) { die("build_hash"); }

    Object key, val;
    for (int i = start_index; i < end_index; i += 2) {
        key = vm->stack[i];
        val = vm->stack[i + 1];
        if (!hashable(key)) {
            table_free(tbl);
            error(&vm->errors, "unusable as hash key: %s",
                    show_object_type(key.type));
            return (Object){};
        }

        key = table_set(tbl, key, val);
        if (key.type == o_Null) { die("build_hash: table_set"); }
    }

    return OBJ(o_Hash, .hash = tbl);
}

static int
call_function(VM *vm, Object fn, int num_args) {
    CompiledFunction *func = fn.data.func;
    if (num_args != func->num_parameters) {
        error_num_args(&vm->errors,
                "function", func->num_parameters, num_args);
        return -1;
    }

    Frame *f = new_frame(vm);
    frame_init(f, *func, vm->sp - num_args);
    vm->sp = f->base_pointer + func->num_locals;
    return 0;
}

static int
call_builtin(VM *vm, Object builtin, int num_args) {
    int base_pointer = vm->sp - num_args;
    Object *args = vm->stack + base_pointer;

    Builtin *fn = builtin.data.ptr;
    return fn(vm, args, num_args);
}

static int
execute_call(VM *vm, int num_args) {
    Object callee = vm->stack[vm->sp - 1 - num_args];
    switch (callee.type) {
        case o_CompiledFunction:
            return call_function(vm, callee, num_args);
        case o_BuiltinFunction:
            return call_builtin(vm, callee, num_args);
        default:
            error(&vm->errors, "calling non-function and non-builtin");
            return -1;
    }
}

int vm_run(VM *vm) {
    Opcode op;
    int err, pos, num;
    Object obj;

    int ip;
    const Builtins *builtins = get_builtins();
    Frame *current_frame = vm->frames;
    Instructions ins = instructions(current_frame);

    while (current_frame->ip < ins.length - 1) {
        ip = ++current_frame->ip;
        op = ins.data[ip];

        switch (op) {
            case OpConstant:
                // constants index
                num = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                err = vm_push_constant(vm, vm->constants.data[num]);
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
                pos = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip = pos - 1;
                break;
            case OpJumpNotTruthy:
                pos = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                obj = vm_pop(vm);
                if (!is_truthy(obj)) {
                    current_frame->ip = pos - 1;
                }
                break;

            case OpNull:
                err = vm_push(vm, _null);
                if (err == -1) { return -1; };
                break;

            case OpSetGlobal:
                // globals index
                pos = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                vm->globals[pos] = vm_pop(vm);
                break;
            case OpGetGlobal:
                // globals index
                pos = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                err = vm_push(vm, vm->globals[pos]);
                if (err == -1) { return -1; };
                break;

            case OpArray:
                // number of array elements
                num = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                obj = build_array(vm, vm->sp - num, vm->sp);
                vm->sp -= num;

                err = vm_push(vm, obj);
                if (err == -1) { return -1; };
                break;

            case OpHash:
                // number of elements
                num = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                obj = build_hash(vm, vm->sp - num, vm->sp);
                if (obj.type == o_Null) { return -1; };
                vm->sp -= num;

                err = vm_push(vm, obj);
                if (err == -1) { return -1; };
                break;

            case OpIndex:
                err = execute_index_expression(vm);
                if (err == -1) { return -1; };
                break;

            case OpCall:
                // num arguments
                num = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                err = execute_call(vm, num);
                if (err == -1) { return -1; };

                current_frame = vm->frames + vm->frames_index - 1;
                ins = instructions(current_frame);
                break;

            case OpReturnValue:
                // return value
                obj = vm_pop(vm);

                pop_frame(vm);
                vm->sp = current_frame->base_pointer - 1;

                err = vm_push(vm, obj);
                if (err == -1) { return -1; };

                current_frame--;
                ins = instructions(current_frame);
                break;

            case OpReturn:
                pop_frame(vm);
                vm->sp = current_frame->base_pointer - 1;

                err = vm_push(vm, _null);
                if (err == -1) { return -1; };

                current_frame--;
                ins = instructions(current_frame);
                break;

            case OpSetLocal:
                // locals index
                pos = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                vm->stack[current_frame->base_pointer + pos] = vm_pop(vm);
                break;
            case OpGetLocal:
                // locals index
                pos = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                err = vm_push(vm, vm->stack[current_frame->base_pointer + pos]);
                if (err == -1) { return -1; };
                break;

            case OpGetBuiltin:
                // builtin fn index
                pos = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                err = vm_push(vm,
                        OBJ(o_BuiltinFunction, .ptr = builtins[pos].fn));
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

static void
mark_alloc(Object obj) {
    assert(obj.type >= o_String);

    // access [Alloc] prepended to ptr with [vm_allocate]
    Alloc *alloc = ((Alloc *)obj.data.ptr) - 1;
    alloc->is_marked = true;
}

static void
trace_mark_object(Object obj) {
    switch (obj.type) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_CompiledFunction:
            return;

        // case o_Error:
        case o_String:
            mark_alloc(obj);
            return;

        case o_Array:
            {
                mark_alloc(obj);

                for (int i = 0; i < obj.data.array->length; i++)
                    trace_mark_object(obj.data.array->data[i]);
                return;
            }

        case o_Hash:
            {
                mark_alloc(obj);

                tbl_it it;
                tbl_iterator(&it, obj.data.hash);
                while (tbl_next(&it)) {
                    trace_mark_object(it.cur_key);
                    trace_mark_object(it.cur_val);
                }
                return;
            }

        default:
            die("trace_mark_object: type %s (%d) not handled",
                    show_object_type(obj.type), obj.type);
    }
}

// free [Alloc]ation
void alloc_free(Alloc *alloc) {
    void *obj_data = alloc + 1;
    switch (alloc->type) {
        case o_String:
            free(((CharBuffer *)obj_data)->data);
            break;

        case o_Array:
            free(((ObjectBuffer *)obj_data)->data);
            break;

        case o_Hash:
            table_free(obj_data);
            break;

        default:
            die("alloc_free: %d typ not handled\n", alloc->type);
            return;
    }
    free(alloc);
}

void vm_free(VM *vm) {
    for (int i = 0; i < vm->allocs.length; i++) {
        alloc_free(vm->allocs.data[i]);
    }
    free(vm->allocs.data);
    for (int i = 0; i < vm->errors.length; i++) {
        free(vm->errors.data[i]);
    }
    free(vm->errors.data);
    free(vm->stack);
    free(vm->globals);
    free(vm->frames);
}

void mark_and_sweep(VM *vm) {
    // mark
    int i;
    Object *stack = vm->stack;
    for (i = vm->sp; i >= 0; i--) {
        if (stack[i].type >= o_String) {
            trace_mark_object(stack[i]);
        }
    }

    // sweep
    Alloc *cur;
    Alloc **allocs = vm->allocs.data;
    int length = vm->allocs.length,
        new_length = length;
    for (i = 0; i < length; i++) {
        cur = allocs[i];
        if (cur->is_marked) {
            cur->is_marked = false;
            continue;
        }

        alloc_free(cur);
        allocs[i] = allocs[--new_length];
    }
    vm->allocs.length = new_length;
}

static void *
allocate(VM *vm, size_t size) {
    vm->bytesTillGC -= size;
    void *ptr = malloc(size);
    if (ptr == NULL) { die("allocate"); }
    return ptr;
}

static void *
vm_allocate(VM *vm, ObjectType type, size_t size) {
    // prepend [Alloc]
    size += sizeof(Alloc);
    if (vm->bytesTillGC <= 0) {
        mark_and_sweep(vm);
        vm->bytesTillGC = NextGC;
    }

    Alloc *ptr = malloc(size);
    if (ptr == NULL) { die("vm_allocate"); }
    ptr->is_marked = false;
    ptr->type = type;
    AllocBufferPush(&vm->allocs, ptr);

    return (Alloc *)(ptr + 1);
}
