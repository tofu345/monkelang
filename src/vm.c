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
#include <stdlib.h>
#include <string.h>

// #define DEBUG_PRINT

const Object _true = BOOL(true);
const Object _false = BOOL(false);

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

Object object_copy(VM* vm, Object obj) {
    switch (obj.type) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_BuiltinFunction:
            return obj;

        case o_String:
            {
                CharBuffer old_str = *obj.data.string;
                CharBuffer* new_str;
                if (old_str.length > 0) {
                    // copy string contents
                    char *new_buf = allocate(vm, old_str.capacity * sizeof(char));
                    memcpy(new_buf, old_str.data, old_str.length * sizeof(char));
                    new_buf[old_str.length] = '\0';

                    // copy allocated [Object]
                    new_str = COMPOUND_OBJ(o_String, CharBuffer, {
                        .data = new_buf,
                        .length = old_str.length,
                        .capacity = old_str.capacity,
                    });
                    return OBJ(o_String, .string = new_str);

                } else {
                    new_str = compound_obj(vm, o_String, sizeof(CharBuffer), NULL);
                }
                return OBJ(o_String, .string = new_str);
            }

        case o_Array:
            {
                ObjectBuffer old_arr = *obj.data.array;
                ObjectBuffer* new_arr;
                if (old_arr.length > 0) {
                    Object *new_buf = allocate(vm, old_arr.capacity * sizeof(Object));
                    // copy contents
                    for (int i = 0; i < old_arr.length; i++) {
                        new_buf[i] = object_copy(vm, old_arr.data[i]);
                    }

                    new_arr = COMPOUND_OBJ(o_Array, ObjectBuffer, {
                        .data = new_buf,
                        .length = old_arr.length,
                        .capacity = old_arr.capacity
                    });
                } else {
                    new_arr = compound_obj(vm, o_Array, sizeof(ObjectBuffer), NULL);
                }
                return OBJ(o_Array, .array = new_arr);
            }

        case o_Hash:
            die("object_copy hash: not implemented");
            return NULL_OBJ;

        default:
            die("object_copy: object type not handled %d\n", obj.type);
            return NULL_OBJ;
    }
}

// return to previous [Frame]
static Frame *
pop_frame(VM *vm) {
    vm->frames_index--;
    return vm->frames + vm->frames_index;
}

// add new frame if less than [MaxFraxes]
static error
push_frame(VM *vm) {
    vm->frames_index++;
    if (vm->frames_index >= MaxFraxes) {
        vm->frames_index = 0;
        return new_error("exceeded maximum function call stack");
    }
    return 0;
}

error vm_push(VM *vm, Object obj) {
    if (vm->sp >= StackSize) {
        return new_error("stack overflow");
    }

    vm->stack[vm->sp++] = obj;
    return 0;
}

Object vm_pop(VM *vm) {
    return vm->stack[--vm->sp];
}

static error
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
            return new_error("unkown float operator: (Opcode: %d)", op);
    }

    return vm_push(vm, OBJ(o_Float, .floating = result));
}

static error
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
            return new_error("unkown integer operator: (Opcode: %d)", op);
    }

    return vm_push(vm, OBJ(o_Integer, .integer = result));
}

static error
execute_binary_string_operation(VM *vm, Opcode op, Object left, Object right) {
    if (op != OpAdd) {
        return new_error("unkown string operator: (Opcode: %d)", op);
    }

    // copy [left] and [right] into new string
    // NOTE: must occur before [compound_object()],
    // if not, potential garbage collection and use after free.
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

    CharBuffer* obj_data = COMPOUND_OBJ(o_String, CharBuffer, {
        .data = result,
        .length = length,
        .capacity = capacity
    });
    return vm_push(vm, OBJ(o_String, .string = obj_data));
}

static error
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

    return new_error("unsupported types for binary operation: %s (Opcode: %d) %s",
            show_object_type(left.type), op, show_object_type(right.type));
}

static error
execute_integer_comparison(VM *vm, Opcode op, Object left, Object right) {
    Object result;

    switch (op) {
        case OpEqual:
            result = BOOL(left.data.integer == right.data.integer);
            break;
        case OpNotEqual:
            result = BOOL(left.data.integer != right.data.integer);
            break;
        case OpGreaterThan:
            result = BOOL(left.data.integer > right.data.integer);
            break;
        default:
            return new_error("unkown integer comparison operator: (Opcode: %d)", op);
    }
    return vm_push(vm, result);
}

static error
execute_float_comparison(VM *vm, Opcode op, Object left, Object right) {
    Object result;

    switch (op) {
        case OpEqual:
            result = BOOL(left.data.floating == right.data.floating);
            break;
        case OpNotEqual:
            result = BOOL(left.data.floating != right.data.floating);
            break;
        case OpGreaterThan:
            result = BOOL(left.data.floating > right.data.floating);
            break;
        default:
            return new_error("unkown float comparison operator: (Opcode: %d)", op);
    }
    return vm_push(vm, result);
}

static error
execute_comparison(VM *vm, Opcode op) {
    Object right = vm_pop(vm);
    Object left = vm_pop(vm);

    if (left.type == o_Integer && right.type == o_Integer) {
        return execute_integer_comparison(vm, op, left, right);

    } else if (left.type == o_Float && right.type == o_Float) {
        return execute_float_comparison(vm, op, left, right);
    }

    Object eq = object_eq(left, right);
    if (IS_ERR(eq)) { return eq.data.err; }

    switch (op) {
        case OpEqual:
            return vm_push(vm, eq);

        case OpNotEqual:
            eq.data.boolean = !eq.data.boolean;
            return vm_push(vm, eq);

        default:
            return new_error("unkown comparison operator: (Opcode: %d) (%s %s)",
                    op, show_object_type(left.type), show_object_type(right.type));
    }
}

static error
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

static error
execute_minus_operator(VM *vm) {
    Object operand = vm_pop(vm);
    switch (operand.type) {
        case o_Integer:
            operand.data.integer = -operand.data.integer;
            break;

        case o_Float:
            operand.data.floating = -operand.data.floating;
            break;

        default:
            return new_error("unsupported type for negation: %s",
                    show_object_type(operand.type));
    }
    return vm_push(vm, operand);
}

static error
execute_array_index(VM *vm, Object array, Object index) {
    ObjectBuffer *arr = array.data.array;
    int i = index.data.integer,
        max = arr->length - 1;
    if (i < 0 || i > max) {
        return vm_push(vm, NULL_OBJ);
    }

    return vm_push(vm, arr->data[i]);
}

static error
execute_hash_index(VM *vm, Object hash, Object index) {
    table *tbl = hash.data.hash;
    if (!hashable(index)) {
        return new_error("unusable as hash key: %s",
                show_object_type(index.type));
    }

    return vm_push(vm, table_get(tbl, index));
}

static error
execute_index_expression(VM *vm) {
    Object index = vm_pop(vm);
    Object left = vm_pop(vm);

    if (left.type == o_Array && index.type == o_Integer) {
        return execute_array_index(vm, left, index);

    } else if (left.type == o_Hash) {
        return execute_hash_index(vm, left, index);

    } else {
        return new_error("index operator not supported: %s[%s]",
                show_object_type(left.type), show_object_type(index.type));
    }
}

static error
execute_set_array_index(Object array, Object index,
        Object elem) {
    ObjectBuffer *arr = array.data.array;
    int i = index.data.integer,
        max = arr->length - 1;
    if (i < 0 || i > max) {
        return new_error("cannot set list index out of range");
    }

    arr->data[i] = elem;
    return 0;
}

static error
execute_set_hash_index(Object hash, Object index, Object elem) {
    table *tbl = hash.data.hash;
    if (!hashable(index)) {
        return new_error("unusable as hash key: %s",
                show_object_type(index.type));
    }

    // [o_Null] value in table represents a lack of a value.
    if (elem.type == o_Null) {
        table_remove(tbl, index);
        return 0;
    }

    Object result = table_set(tbl, index, elem);
    if (result.type == o_Null) {
        return new_error("could not set hash index");
    }
    return 0;
}

static error
execute_set_index(VM *vm) {
    Object index = vm_pop(vm);
    Object left = vm_pop(vm);
    Object right = vm_pop(vm);

    if (left.type == o_Array && index.type == o_Integer) {
        return execute_set_array_index(left, index, right);

    } else if (left.type == o_Hash) {
        return execute_set_hash_index(left, index, right);

    } else {
        return new_error("index assignment operator not supported: %s[%s]",
                show_object_type(left.type), show_object_type(index.type));
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

static error
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

                CharBuffer* str_buf = COMPOUND_OBJ(o_String, CharBuffer, {
                    .data = buf,
                    .length = str.length,
                    .capacity = capacity
                });
                return vm_push(vm, OBJ(o_String, .string = str_buf));
            }

        default:
            die("vm_push_constant: type %d not handled", c.type);
            return 0;
    }
}

static Object
build_array(VM *vm, int start_index, int end_index) {
    int length = end_index - start_index;
    ObjectBuffer *elems =
        compound_obj(vm, o_Array, sizeof(ObjectBuffer), NULL);

    if (length > 0) {
        elems->length = length;
        elems->capacity = power_of_2_ceil(length);
        int size = elems->capacity * sizeof(Object);
        elems->data = allocate(vm, size);
        memcpy(elems->data, vm->stack + start_index, length * sizeof(Object));
    }

    return OBJ(o_Array, .array = elems);
}

static Object
build_hash(VM *vm, int start_index, int end_index) {
    table *tbl = compound_obj(vm, o_Hash, sizeof(table), NULL);
    if (table_init(tbl) == NULL) { die("build_hash"); }

    Object key, val, res;
    for (int i = start_index; i < end_index; i += 2) {
        key = vm->stack[i];
        val = vm->stack[i + 1];

        if (!hashable(key)) {
            return ERR("unusable as hash key: %s",
                    show_object_type(key.type));
        }

        if (val.type == o_Null) { continue; }

        res = table_set(tbl, key, val);
        if (res.type == o_Null) {
            return ERR("could not set as hash value: %s",
                    show_object_type(val.type));
        }
    }

    return OBJ(o_Hash, .hash = tbl);
}

static error
call_closure(VM *vm, Closure *cl, int num_args) {
    Function *fn = cl->func;
    if (num_args != fn->num_parameters) {
        if (fn->name) {
            return error_num_args(fn->name, fn->num_parameters, num_args);
        }
        return error_num_args("function", fn->num_parameters, num_args);
    }

    error err = push_frame(vm);
    if (err) { return err; }

    // init new frame
    int base_pointer = vm->sp - num_args;
    Frame *f = vm->frames + vm->frames_index;
    frame_init(f, cl, base_pointer);

    if (fn->num_locals > 0) {
        // set local variables to [o_Null].
        memset(vm->stack + vm->sp, 0, fn->num_locals * sizeof(Object));
    }

    vm->sp = base_pointer + fn->num_locals;

    return 0;
}

static error
call_builtin(VM *vm, Builtin *builtin, int num_args) {
    Object *args = vm->stack + vm->sp - num_args;
    Object result = builtin->fn(vm, args, num_args);

    // remove arguments and [Builtin] from stack.
    vm->sp -= num_args + 1;

    switch (result.type) {
        case o_Error:
            return result.data.err;
        case o_Null:
            return vm_push(vm, NULL_OBJ);
        default:
            return vm_push(vm, result);
    }
}

static error
execute_call(VM *vm, int num_args) {
    Object callee = vm->stack[vm->sp - 1 - num_args];

#ifdef DEBUG_PRINT
    printf("call: ");
    object_fprint(callee, stdout);
    putc('\n', stdout);
#endif

    switch (callee.type) {
        case o_Closure:
            return call_closure(vm, callee.data.closure, num_args);
        case o_BuiltinFunction:
            return call_builtin(vm, callee.data.ptr, num_args);
        default:
            return new_error("calling non-function and non-builtin");
    }
}

static error
vm_push_closure(VM *vm, int const_index, int num_free) {
    Constant constant = vm->constants.data[const_index];

    if (constant.type != c_Function) {
        return new_error("not a function: constant %d", const_index);
    }

    size_t free_size = num_free * sizeof(Object),
           size = sizeof(Closure) + free_size;

#ifdef DEBUG_PRINT
    printf("creating closure: %s\n", constant.data.function->name);
#endif

    Closure *closure = compound_obj(vm, o_Closure, size, NULL);
    closure->num_free = num_free;
    closure->func = constant.data.function;

    if (num_free > 0) {
        vm->sp -= num_free;
        memcpy(closure->free, vm->stack + vm->sp, free_size);
    }
    return vm_push(vm, OBJ(o_Closure, .closure = closure));
}

error vm_run(VM *vm, Bytecode bytecode) {
    Function main_fn = {
        .instructions = *bytecode.instructions,
        .num_locals = 0,
        .num_parameters = 0,
        .name = "<main>"
    };
    Closure main_closure = { .func = &main_fn, .num_free = 0 };
    frame_init(vm->frames, &main_closure, 0);
    vm->num_globals = bytecode.num_globals;
    vm->constants = *bytecode.constants;

    Frame *current_frame = vm->frames;
    Instructions ins = instructions(current_frame);

    const Builtin *builtins = get_builtins();
    Opcode op;
    int ip, pos, num;
    Object obj;
    error err;
    while (current_frame->ip < ins.length - 1) {
        ip = ++current_frame->ip;
        op = ins.data[ip];

        switch (op) {
            case OpConstant:
                // constant index
                pos = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                err = vm_push_constant(vm, vm->constants.data[pos]);
                if (err) { return err; };
                break;

            case OpAdd:
            case OpSub:
            case OpMul:
            case OpDiv:
                err = execute_binary_operation(vm, op);
                if (err) { return err; };
                break;

            case OpPop:
                vm_pop(vm);
                break;

            case OpTrue:
                err = vm_push(vm, _true);
                if (err) { return err; };
                break;
            case OpFalse:
                err = vm_push(vm, _false);
                if (err) { return err; };
                break;

            case OpEqual:
            case OpNotEqual:
            case OpGreaterThan:
                err = execute_comparison(vm, op);
                if (err) { return err; };
                break;

            case OpBang:
                err = execute_bang_operator(vm);
                if (err) { return err; };
                break;
            case OpMinus:
                err = execute_minus_operator(vm);
                if (err) { return err; };
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
                err = vm_push(vm, NULL_OBJ);
                if (err) { return err; };
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
                if (err) { return err; };
                break;

            case OpArray:
                // number of array elements
                num = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                obj = build_array(vm, vm->sp - num, vm->sp);
                vm->sp -= num;

                err = vm_push(vm, obj);
                if (err) { return err; };
                break;

            case OpHash:
                // number of elements
                num = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                obj = build_hash(vm, vm->sp - num, vm->sp);
                if (IS_ERR(obj)) { return obj.data.err; };
                vm->sp -= num;

                err = vm_push(vm, obj);
                if (err) { return err; };
                break;

            case OpIndex:
                err = execute_index_expression(vm);
                if (err) { return err; };
                break;

            case OpSetIndex:
                err = execute_set_index(vm);
                if (err) { return err; };
                break;

            case OpCall:
                // num arguments
                num = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                err = execute_call(vm, num);
                if (err) { return err; };

                current_frame = vm->frames + vm->frames_index;
                ins = instructions(current_frame);
                break;

            case OpReturnValue:
                // return value
                obj = vm_pop(vm);

                // remove arguments and [Function] from stack.
                vm->sp = current_frame->base_pointer - 1;

                current_frame = pop_frame(vm);
                ins = instructions(current_frame);

                err = vm_push(vm, obj);
                if (err) { return err; };
                break;

            case OpReturn:
                // remove arguments and [Function] from stack.
                vm->sp = current_frame->base_pointer - 1;

                current_frame = pop_frame(vm);
                ins = instructions(current_frame);

                err = vm_push(vm, NULL_OBJ);
                if (err) { return err; };
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
                if (err) { return err; };
                break;

            case OpGetBuiltin:
                // builtin fn index
                pos = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                err = vm_push(vm,
                        OBJ(o_BuiltinFunction, .builtin = builtins + pos));
                if (err) { return err; };
                break;

            case OpClosure:
                // constant index
                pos = read_big_endian_uint16(ins.data + ip + 1);
                // num free variables
                num = read_big_endian_uint8(ins.data + ip + 3);
                current_frame->ip += 3;

                err = vm_push_closure(vm, pos, num);
                if (err) { return err; };
                break;

            case OpGetFree:
                // free variable index
                pos = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                err = vm_push(vm, current_frame->cl->free[pos]);
                if (err) { return err; };
                break;

            case OpCurrentClosure:
                err = vm_push(vm,
                        OBJ(o_Closure, .closure = current_frame->cl));
                if (err) { return err; };
                break;

            default:
                return new_error("unknown opcode %d", op);
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

#ifdef DEBUG_PRINT
    printf("mark: ");
    object_fprint(obj, stdout);
    putc('\n', stdout);
#endif

    // access [Alloc] prepended to ptr with [vm_allocate]
    Allocation *alloc = ((Allocation *)obj.data.ptr) - 1;
    alloc->is_marked = true;
}

static void
trace_mark_object(Object obj) {
    switch (obj.type) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_BuiltinFunction:
        case o_Error:
            return;

        case o_String:
            mark_alloc(obj);
            return;

        case o_Closure:
            mark_alloc(obj);
            for (int i = 0; i < obj.data.closure->num_free; i++)
                trace_mark_object(obj.data.closure->free[i]);
            return;

        case o_Array:
            mark_alloc(obj);
            for (int i = 0; i < obj.data.array->length; i++)
                trace_mark_object(obj.data.array->data[i]);
            return;

        case o_Hash:
            mark_alloc(obj);

            tbl_it it;
            tbl_iterator(&it, obj.data.hash);
            while (tbl_next(&it)) {
                trace_mark_object(it.cur_key);
                trace_mark_object(it.cur_val);
            }
            return;

        default:
            die("trace_mark_object: type %s (%d) not handled",
                    show_object_type(obj.type), obj.type);
    }
}

static void
free_allocation(Allocation *alloc) {
    assert(alloc->type >= o_String);

    void *obj_data = alloc->object_data;

#ifdef DEBUG_PRINT
    printf("free: ");
    object_fprint(OBJ(alloc->type, .ptr = alloc + 1), stdout);
    putc('\n', stdout);
#endif

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

        case o_Closure:
            break;

        default:
            die("alloc_free: %d typ not handled\n", alloc->type);
            return;
    }
    free(alloc);
}

void vm_free(VM *vm) {
    Allocation *next, *cur = vm->last;
    while (cur) {
        next = cur->next;
        free_allocation(cur);
        cur = next;
    }
    free(vm->stack);
    free(vm->globals);
    free(vm->frames);
}

void mark_objs(Object *objs, int len) {
    for (int i = 0; i < len; i++) {
        if (objs[i].type >= o_String) {
            trace_mark_object(objs[i]);
        }
    }
}

void mark_and_sweep(VM *vm) {
    mark_objs(vm->stack, vm->sp);
    mark_objs(vm->globals, vm->num_globals);

    // sweep and rebuild Linked list of [Allocations].
    Allocation *cur = vm->last,
               *prev_marked = NULL;
    while (cur) {
        Allocation *next = cur->next;

        if (cur->is_marked) {
            cur->is_marked = false;
            cur->next = prev_marked;
            prev_marked = cur;
            cur = next;
            continue;
        }

        free_allocation(cur);
        cur = next;
    }
    vm->last = prev_marked;
}

void *allocate(VM *vm, size_t size) {
    vm->bytesTillGC -= size;
    void *ptr = malloc(size);
    if (ptr == NULL) { die("allocate"); }
    return ptr;
}

void *compound_obj(VM *vm, ObjectType type, size_t size, void *data) {
    // prepend [Alloc] object.
    size_t actual_size = size + sizeof(Allocation);

    vm->bytesTillGC -= actual_size;
    if (vm->bytesTillGC <= 0) {
#ifdef DEBUG_PRINT
        printf("create object: %s\n", show_object_type(type));
        printf("call stack:\n");
        for (int i = 0; i <= vm->frames_index; i++) {
            printf("-> function: %s\n", vm->frames[i].cl->func->name);
        }
        putc('\n', stdout);
#endif

        mark_and_sweep(vm);
        vm->bytesTillGC = NextGC;
    }

    Allocation *ptr = malloc(actual_size);
    if (ptr == NULL) { die("malloc"); }
    *ptr = (Allocation){
        .is_marked = false,
        .type = type,
        .next = vm->last,
    };
    vm->last = ptr;

    void *obj_data = ptr->object_data;
    if (data) {
        memcpy(obj_data, data, size);
    } else {
        memset(obj_data, 0, size);
    }
    return obj_data;
}
