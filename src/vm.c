#include "vm.h"
#include "ast.h"
#include "builtin.h"
#include "code.h"
#include "constants.h"
#include "errors.h"
#include "object.h"
#include "table.h"
#include "utils.h"
#include "allocation.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// initialize `vm.frames[vm.frames_index]`
static void
frame_init(VM *vm, Closure *cl, int base_pointer) {
    vm->frames[vm->frames_index] = (Frame) {
        .cl = cl,
        .ip = -1,
        .base_pointer = base_pointer,
    };
}

static Instructions
instructions(Frame *f) {
    return f->cl->func->instructions;
}

static error
error_unknown_operation(Opcode op, Object left, Object right) {
    static const char *op_strs[] = {"+", "-", "*", "/", "==", "!=", "<", ">"};
    if (op < OpAdd || op > OpGreaterThan) {
        return new_error("unkown operation: %s %s %s",
                lookup(op)->name, show_object_type(left.type),
                show_object_type(right.type));
    }

    return new_error("unkown operation: %s %s %s",
            show_object_type(left.type), op_strs[op - OpAdd],
            show_object_type(right.type));
}

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

    if (vm->main_cl == NULL) {
        vm->main_cl = calloc(1, sizeof(Closure));
        if (vm->main_cl == NULL) { die("malloc"); }
    }
}

void vm_free(VM *vm) {
#ifdef DEBUG
    if (vm->last) { puts("\ncleaning up:"); }
#endif

    Allocation *next, *cur = vm->last;
    while (cur) {
        next = cur->next;
        free_allocation(cur);
        cur = next;
    }
    free(vm->stack);
    free(vm->globals);
    free(vm->frames);
    free(vm->main_cl);
}

// return to previous [Frame]
static Frame *
pop_frame(VM *vm) {
    vm->frames_index--;
    return vm->frames + vm->frames_index;
}

// increment [vm.frames_index] if less than [MaxFraxes]
static error
new_frame(VM *vm) {
    ++vm->frames_index;
    if (vm->frames_index >= MaxFraxes) {
        --vm->frames_index;
        return new_error("exceeded maximum function call stack");
    }
    return 0;
}

static error
vm_push(VM *vm, Object obj) {
    if (vm->sp >= StackSize) {
        return new_error("stack overflow");
    }

    vm->stack[vm->sp++] = obj;
    return 0;
}

static Object
vm_pop(VM *vm) {
#ifdef DEBUG
    --vm->sp;

    // check for possible local variables overwrite
    Frame *cur = &vm->frames[vm->frames_index];
    if (vm->sp >= cur->base_pointer + cur->cl->func->num_parameters) {
        return vm->stack[vm->sp];
    }

    print_vm_stack_trace(vm);
    die("stack pointer below limit for current frame");

#else
    return vm->stack[--vm->sp];
#endif
}

#ifdef DEBUG
static void
debug_print_args(Object function, Object *args, int num_args) {
    object_fprint(function, stdout);
    printf(" (");
    int last = num_args - 1;
    for (int i = 0; i < last; i++) {
        object_fprint(args[i], stdout);
        printf(", ");
    }
    if (num_args >= 1) {
        object_fprint(args[last], stdout);
    }
    printf(")");
}

static void
debug_print_create(Object new_obj) {
    printf("create: ");
    object_fprint(new_obj, stdout);
    putc('\n', stdout);
}
#endif

static Object
execute_binary_float_operation(Opcode op, Object left, Object right) {
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
            die("unknown binary float operator: %s (%d)",
                    lookup(op)->name, op);
    }

    return OBJ(o_Float, .floating = result);
}

static Object
execute_binary_integer_operation(Opcode op, Object left, Object right) {
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
            die("unknown binary integer operator: %s (%d)",
                    lookup(op)->name, op);
    }

    return OBJ(o_Integer, .integer = result);
}

static Object
execute_binary_string_operation(VM *vm, Opcode op, Object left, Object right) {
    if (op != OpAdd) {
        return OBJ(o_Error, .err = error_unknown_operation(op, left, right));
    }

    // copy [left] and [right] into new string
    CharBuffer *l = left.data.string,
               *r = right.data.string;

    int length = l->length + r->length;

    CharBuffer *new_str = create_string(vm, NULL, length);

    memcpy(new_str->data, l->data, l->length * sizeof(char));
    memcpy(new_str->data + l->length, r->data,
            r->length * sizeof(char));
    new_str->data[length] = '\0';

    Object obj = OBJ(o_String, .string = new_str);

#ifdef DEBUG
    debug_print_create(obj);
#endif

    return obj;
}

static Object
execute_binary_array_operation(VM *vm, Opcode op, Object left, Object right) {
    if (op != OpMul) {
        return OBJ(o_Error, .err = error_unknown_operation(op, left, right));
    }

    int l_length = left.data.array->length,
        new_length = l_length * right.data.integer;

    ObjectBuffer *new_arr = create_array(vm, NULL, new_length);
    for (int i = 0; i < new_length; i += l_length) {
        memcpy(new_arr->data + i, left.data.array->data, l_length * sizeof(Object));
    }

    Object obj = OBJ(o_Array, .array = new_arr);

#ifdef DEBUG
    debug_print_create(obj);
#endif

    return obj;
}

static error
execute_binary_operation(VM *vm, Opcode op) {
    // not [vm_pop()] to keep left and right in scope, in case GC is triggered.
    Object right = vm->stack[vm->sp - 1];
    Object left = vm->stack[vm->sp - 2];
    Object result;

    // TODO replace with switch case

    if (left.type == o_Integer && right.type == o_Integer) {
        result = execute_binary_integer_operation(op, left, right);

    } else if (left.type == o_Float && right.type == o_Float) {
        result = execute_binary_float_operation(op, left, right);

    } else if (left.type == o_String && right.type == o_String) {
        result = execute_binary_string_operation(vm, op, left, right);

    } else if (left.type == o_Array && right.type == o_Integer) {
        result = execute_binary_array_operation(vm, op, left, right);

    } else {
        return error_unknown_operation(op, left, right);
    }

    vm->sp -= 2;

    if (IS_ERR(result)) {
        return result.data.err;
    }
    return vm_push(vm, result);
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
        case OpLessThan:
            result = BOOL(left.data.integer < right.data.integer);
            break;
        case OpGreaterThan:
            result = BOOL(left.data.integer > right.data.integer);
            break;
        default:
            die("unkown integer comparison operator: %s (%d)",
                    lookup(op)->name, op);
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
        case OpLessThan:
            result = BOOL(left.data.floating < right.data.floating);
            break;
        case OpGreaterThan:
            result = BOOL(left.data.floating > right.data.floating);
            break;
        default:
            die("unkown float comparison operator: %s (%d)",
                    lookup(op)->name, op);
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

    Object res = object_eq(left, right);
    if (IS_NULL(res)) {
        return new_error("cannot compare: '%s' with '%s'",
                show_object_type(left.type),
                show_object_type(right.type));
    }

    switch (op) {
        case OpEqual:
            return vm_push(vm, res);

        case OpNotEqual:
            res.data.boolean = !res.data.boolean;
            return vm_push(vm, res);

        default:
            return error_unknown_operation(op, left, right);
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
            return vm_push(vm, BOOL(true));
        default:
            return vm_push(vm, BOOL(false));
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
execute_table_index(VM *vm, Object obj, Object index) {
    Table *tbl = obj.data.table;
    if (!hashable(index)) {
        return new_error("unusable as table key: %s",
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

    } else if (left.type == o_Table) {
        return execute_table_index(vm, left, index);

    } else {
        return new_error("cannot index %s with %s",
                show_object_type(left.type),
                show_object_type(index.type));
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
execute_set_table_index(Object obj, Object index, Object val) {
    Table *tbl = obj.data.table;
    if (!hashable(index)) {
        return new_error("unusable as table key: %s",
                show_object_type(index.type));
    }

    // [table] cannot have [o_Null] key or value.
    if (val.type == o_Null) {
        table_remove(tbl, index);
        return 0;
    }

    Object result = table_set(tbl, index, val);
    if (result.type == o_Null) {
        return new_error("could not set table index");
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

    } else if (left.type == o_Table) {
        return execute_set_table_index(left, index, right);

    } else {
        return new_error("index assignment not supported for %s[%s]",
                show_object_type(left.type), show_object_type(index.type));
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

                Token *str_tok = c.data.string;
                CharBuffer *new_str =
                    create_string(vm, str_tok->start, str_tok->length);
                Object obj = OBJ(o_String, .string = new_str);

#ifdef DEBUG
    debug_print_create(obj);
#endif

                return vm_push(vm, obj);
            }

        default:
            die("vm_push_constant: type %d not handled", c.type);
            return 0;
    }
}

static Object
build_array(VM *vm, int start_index, int end_index) {
    int length = end_index - start_index;
    Object *data = NULL;

    if (length > 0) {
        data = vm->stack + start_index;
    }

    Object array = OBJ(o_Array, .array = create_array(vm, data, length));

#ifdef DEBUG
    debug_print_create(array);
#endif

    return array;
}

static Object
build_table(VM *vm, int start_index, int end_index) {
    Table *tbl = create_table(vm);

    Object key, val;
    for (int i = start_index; i < end_index; i += 2) {
        key = vm->stack[i];
        val = vm->stack[i + 1];

        if (!hashable(key)) {
            return ERR("cannot use type as table key: %s",
                    show_object_type(key.type));
        }

        // [table] cannot have [o_Null] key or value.
        if (val.type == o_Null) {
            continue;
        }

        // FIXME: die() instead?
        // should only happen if allocation of bucket overflow fails.
        if (IS_NULL(table_set(tbl, key, val))) {
            return ERR("could not set table value: %s",
                    show_object_type(val.type));
        }
    }

    Object obj = OBJ(o_Table, .table = tbl);

#ifdef DEBUG
    debug_print_create(obj);
#endif
    return obj;
}

static error
call_closure(VM *vm, Closure *cl, int num_args) {
    CompiledFunction *fn = cl->func;
    if (num_args != fn->num_parameters) {
        FunctionLiteral *lit = fn->literal;
        if (lit->name) {
            Identifier *id = lit->name;
            return new_error("%.*s takes %d argument%s got %d",
                    LITERAL(id->tok),
                    fn->num_parameters, fn->num_parameters != 1 ? "s" : "",
                    num_args);
        };

        return error_num_args("<anonymous function>",
                fn->num_parameters, num_args);
    }

    error err = new_frame(vm);
    if (err) { return err; }

    int base_pointer = vm->sp - num_args;
    frame_init(vm, cl, base_pointer);

    if (fn->num_locals > 0) {
        // set local variables to [o_Null] to avoid use after free if GC is
        // triggered and accesses freed Compound Data Types still on the stack.
        memset(vm->stack + vm->sp, 0, fn->num_locals * sizeof(Object));
    }

    vm->sp = base_pointer + fn->num_locals;

    return 0;
}

static error
call_builtin(VM *vm, const Builtin *builtin, int num_args) {
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

#ifdef DEBUG
    printf("call: ");
    Object *args = vm->stack + vm->sp - num_args;
    debug_print_args(callee, args, num_args);
    putc('\n', stdout);
#endif

    switch (callee.type) {
        case o_Closure:
            return call_closure(vm, callee.data.closure, num_args);
        case o_BuiltinFunction:
            return call_builtin(vm, callee.data.builtin, num_args);
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

    CompiledFunction *func = constant.data.function;
    Object *free_variables = &vm->stack[vm->sp - num_free];
    Closure *closure = create_closure(vm, func, free_variables, num_free);
    Object obj = OBJ(o_Closure, .closure = closure);

#ifdef DEBUG
    debug_print_create(obj);
#endif

    // remove free variables from stack
    vm->sp -= num_free;

    return vm_push(vm, obj);
}

error vm_run(VM *vm, Bytecode bytecode) {
    vm->num_globals = bytecode.num_globals;
    vm->constants = *bytecode.constants;

    vm->main_cl->func = bytecode.main_function;
    vm->frames_index = 0;
    frame_init(vm, vm->main_cl, 0);

    Frame *current_frame = vm->frames;
    Instructions ins = instructions(current_frame);
    const Builtin *builtins = get_builtins(NULL);

    int ip, pos, num;
    Opcode op;
    Object obj;
    error err;
    while (current_frame->ip < ins.length - 1) {
        ip = ++current_frame->ip;
        op = ins.data[ip];

        // TODO: use computed gotos
        // https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables
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
                err = vm_push(vm, BOOL(true));
                if (err) { return err; };
                break;
            case OpFalse:
                err = vm_push(vm, BOOL(false));
                if (err) { return err; };
                break;

            case OpEqual:
            case OpNotEqual:
            case OpLessThan:
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

            case OpTable:
                // number of elements
                num = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                obj = build_table(vm, vm->sp - num, vm->sp);
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

#ifdef DEBUG
                printf("return: ");
                debug_print_args(
                        OBJ(o_Closure, .closure = current_frame->cl),
                        vm->stack + vm->sp + 1,
                        current_frame->cl->func->num_parameters);
                printf(" -> ");
                object_fprint(obj, stdout);
                putc('\n', stdout);
#endif

                current_frame = pop_frame(vm);
                ins = instructions(current_frame);

                err = vm_push(vm, obj);
                if (err) { return err; };
                break;

            case OpReturn:
                // remove arguments and [Function] from stack.
                vm->sp = current_frame->base_pointer - 1;

#ifdef DEBUG
                printf("return: ");
                debug_print_args(
                        OBJ(o_Closure, .closure = current_frame->cl),
                        vm->stack + vm->sp + 1,
                        current_frame->cl->func->num_parameters);
                printf(" -> null");
                putc('\n', stdout);
#endif

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
            case OpSetFree:
                // free variable index
                pos = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                current_frame->cl->free[pos] = vm_pop(vm);
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

Object vm_last_popped(VM *vm) {
    return vm->stack[vm->sp];
}

static void
_print_function_name(Closure *cl) {
    FunctionLiteral *lit = cl->func->literal;
    if (lit == NULL) {
        printf("<main function>");
    } else if (lit->name) {
        Identifier *id = lit->name;
        printf("<function: %.*s>", LITERAL(id->tok));
    } else {
        printf("<anonymous function>");
    }
}

static Token *
_token(SourceMapping *mapping) {
    switch (mapping->node.typ) {
        case n_LetStatement:
            {
                LetStatement *ls = mapping->node.obj;
                return node_token(ls->value);
            }
        default:
            return node_token(mapping->node);
    }
}

static void
_print_repeats(int first_idx, int cur_idx) {
    int repeats = cur_idx - first_idx - 1;
    if (repeats > 1) {
        printf("[Previous line repeated %d more times]\n", repeats);
    }
}

void print_vm_stack_trace(VM *vm) {
    // Similar to Python, only print a position in the source where recursion
    // repeats only once.

    Closure *prev = NULL;
    int prev_ip = -1,
        prev_idx = 0;

    int idx;
    SourceMapping *mapping = NULL;
    for (idx = 0; idx <= vm->frames_index; ++idx) {
        Frame frame = vm->frames[idx];

        if (frame.cl == prev && frame.ip == prev_ip) {
            // [frame] stopped at the same position as previous.
            continue;

        } else {
            // previous Frame if present, stopped at the same position as
            // current.
            mapping = find_mapping(frame.cl->func->mappings, frame.ip);

            _print_repeats(prev_idx, idx);
            prev = frame.cl;
            prev_idx = idx;
            prev_ip = frame.ip;
        }

        _print_function_name(frame.cl);

        Token *tok = _token(mapping);
        printf(", line %d\n", tok->line);
        highlight_token(tok, /* leftpad */ 2);
    }

    _print_repeats(prev_idx, idx);
}
