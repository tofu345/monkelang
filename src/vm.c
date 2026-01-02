#include "allocation.h"
#include "ast.h"
#include "builtin.h"
#include "code.h"
#include "compiler.h"
#include "constants.h"
#include "errors.h"
#include "hash-table/ht.h"
#include "object.h"
#include "shared.h"
#include "sub_modules.h"
#include "table.h"
#include "utils.h"
#include "vm.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// load sub module if not done previously and add to `vm.modules`.
error require_sub_module(VM *vm, Constant filename);

// initialize `vm.frames[vm.frames_index]`
static void frame_init(VM *vm, Object function, int base_pointer);
static Instructions frame_instructions(Frame *f);

static error
error_unknown_operation(Opcode op, Object left, Object right) {
    static const char *op_strs[] = {"+", "-", "*", "/", "==", "!=", "<", ">"};
    if (op < OpAdd || op > OpGreaterThan) {
        return errorf("unkown operation: %s %s %s",
                lookup(op)->name, show_object_type(left.type),
                show_object_type(right.type));
    }

    return errorf("unkown operation: %s %s %s",
            show_object_type(left.type), op_strs[op - OpAdd],
            show_object_type(right.type));
}

void vm_init(VM *vm, Compiler *compiler) {
    memset(vm, 0, sizeof(VM));

    vm->compiler = compiler;

    vm->stack = calloc(StackSize, sizeof(Object));
    if (vm->stack == NULL) { die("vm stack create:"); }

    vm->frames = calloc(MaxFraxes, sizeof(Frame));
    if (vm->frames == NULL) { die("vm frames create:"); }

    vm->bytesTillGC = NextGC;

    vm->cur_module = NULL;
    vm->sub_modules = ht_create();
    if (vm->sub_modules == NULL) { die("vm module table create:"); }

    vm->closure = calloc(1, sizeof(Closure));
    if (vm->closure == NULL) { die("vm closure create:"); }
}

inline static void module_free(Module *m) {
    free((char *) m->name);
    free((char *) m->source);
    free_function(m->main_function);
    program_free(&m->program);
    free(m->globals);
    free(m);
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
    free(vm->frames);
    free(vm->closure);
    free(vm->globals);

    hti it = ht_iterator(vm->sub_modules);
    while (ht_next(&it)) {
        module_free(it.current->value);
    }
    ht_destroy(vm->sub_modules);

    memset(vm, 0, sizeof(VM));
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
        return errorf("exceeded maximum function call stack");
    }
    return 0;
}

static error
vm_push(VM *vm, Object obj) {
    if (vm->sp >= StackSize) {
        return errorf("stack overflow");
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
    Object function = cur->function;

    int num_args = 0;
    if (function.type == o_Closure) {
        num_args = function.data.closure->func->num_parameters;
    }
    if (vm->sp >= cur->base_pointer + num_args) {
        return vm->stack[vm->sp];
    }

    print_vm_stack_trace(vm, stdout);
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
debug_print_return(Object function, Object *args, Object return_value) {
    int num_args = 0;
    if (function.type == o_Closure) {
        num_args = function.data.closure->func->num_parameters;
    }

    printf("return: ");
    debug_print_args(function, args, num_args);
    printf(" -> ");
    object_fprint(return_value, stdout);
    putc('\n', stdout);
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
            if (right.data.integer == 0) { return OBJ_ERR("division by zero"); }
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
            if (__builtin_add_overflow(left.data.integer, right.data.integer,
                                       &result)) {
                return OBJ_ERR("integer overflow");
            }
            break;
        case OpSub:
            if (__builtin_sub_overflow(left.data.integer, right.data.integer,
                                       &result)) {
                return OBJ_ERR("integer underflow");
            }
            break;
        case OpMul:
            if (__builtin_mul_overflow(left.data.integer, right.data.integer,
                                       &result)) {
                return OBJ_ERR("integer overflow: multiplication");
            }
            break;
        case OpDiv:
            if (right.data.integer == 0) { return OBJ_ERR("division by zero"); }
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

    if (result.type == o_Error) {
        return result.data.err;
    }
    return vm_push(vm, result);
}

static error
execute_integer_comparison(VM *vm, Opcode op, Object left, Object right) {
    Object result;

    switch (op) {
        case OpEqual:
            result = OBJ_BOOL(left.data.integer == right.data.integer);
            break;
        case OpNotEqual:
            result = OBJ_BOOL(left.data.integer != right.data.integer);
            break;
        case OpLessThan:
            result = OBJ_BOOL(left.data.integer < right.data.integer);
            break;
        case OpGreaterThan:
            result = OBJ_BOOL(left.data.integer > right.data.integer);
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
            result = OBJ_BOOL(left.data.floating == right.data.floating);
            break;
        case OpNotEqual:
            result = OBJ_BOOL(left.data.floating != right.data.floating);
            break;
        case OpLessThan:
            result = OBJ_BOOL(left.data.floating < right.data.floating);
            break;
        case OpGreaterThan:
            result = OBJ_BOOL(left.data.floating > right.data.floating);
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
        case o_Nothing:
            return vm_push(vm, OBJ_BOOL(true));
        default:
            return vm_push(vm, OBJ_BOOL(false));
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
            return errorf("unsupported type for negation: %s",
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
        return vm_push(vm, OBJ_NOTHING);
    }

    return vm_push(vm, arr->data[i]);
}

static error
execute_table_index(VM *vm, Object obj, Object index) {
    Table *tbl = obj.data.table;
    if (!hashable(index)) {
        return errorf("unusable as table key: %s",
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
        return errorf("cannot index %s with %s",
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
        return errorf("cannot set list index out of range");
    }

    arr->data[i] = elem;
    return 0;
}

static error
execute_set_table_index(Object obj, Object index, Object val) {
    Table *tbl = obj.data.table;
    if (!hashable(index)) {
        return errorf("unusable as table key: %s",
                show_object_type(index.type));
    }

    // [table] cannot have [o_Nothing] key or value.
    if (val.type == o_Nothing) {
        table_remove(tbl, index);
        return 0;
    }

    Object result = table_set(tbl, index, val);
    if (result.type == o_Nothing) {
        return errorf("could not set table index");
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
        return errorf("index assignment not supported for %s[%s]",
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

    Object key, val, res;
    for (int i = start_index; i < end_index; i += 2) {
        key = vm->stack[i];
        val = vm->stack[i + 1];

        if (!hashable(key)) {
            return OBJ_ERR("cannot use type as table key: %s",
                    show_object_type(key.type));
        }

        // [table] cannot have [o_Nothing] key or value.
        if (val.type == o_Nothing) {
            continue;
        }

        // FIXME: die() instead?
        // should only happen if allocation of bucket overflow fails.
        res = table_set(tbl, key, val);
        if (res.type == o_Error) {
            return OBJ_ERR("could not set table value: %s",
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
        Identifier *id = fn->literal->name;
        if (id) {
            return error_num_args_(&id->tok, fn->num_parameters, num_args);
        }
        return error_num_args("<anonymous function>", fn->num_parameters, num_args);
    }

    error err = new_frame(vm);
    if (err) { return err; }

    int base_pointer = vm->sp - num_args;
    frame_init(vm, OBJ(o_Closure, .closure = cl), base_pointer);

    if (fn->num_locals > 0) {
        if (vm->sp + fn->num_locals >= StackSize) {
            return errorf("stack overflow: insufficient space for local variables");
        }

        // set local variables to [o_Nothing] to avoid use after free if GC is
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

    vm->sp -= num_args;

    if (result.type == o_Error) {
        return result.data.err;
    } else {
        return vm_push(vm, result);
    }
}

static error
execute_call(VM *vm, int num_args) {
    Object callee = vm_pop(vm);

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
            return errorf("calling non-function and non-builtin");
    }
}

static error
vm_push_closure(VM *vm, Constant constant, int num_free) {
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

static inline void resize_vm_globals(VM *vm, int num_globals) {
    if (num_globals != vm->num_globals) {
        vm->globals = realloc(vm->globals, num_globals * sizeof(Object));
        if (vm->globals == NULL) { die("vm resize globals:"); }
    }

    int added = num_globals - vm->num_globals;
    if (added > 0) {
        Object *globals = vm->globals + vm->num_globals;
        memset(globals, 0, added * sizeof(Object));
    }
    vm->num_globals = num_globals;
}

error vm_run(VM *vm, Bytecode code) {
    resize_vm_globals(vm, code.num_globals);
    vm->closure->func = code.main_function;
    frame_init(vm, OBJ(o_Closure, .closure = vm->closure), 0);

    // frequently accessed
    Frame *current_frame = &vm->frames[vm->frames_index];
    Instructions ins = code.main_function->instructions;
    Constant *constants = vm->compiler->constants.data;
    Object *globals = vm->globals;

    int ip, pos, num;
    const Builtin *builtins = get_builtins(&pos);
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

                err = vm_push_constant(vm, constants[pos]);
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
                err = vm_push(vm, OBJ_BOOL(true));
                if (err) { return err; };
                break;
            case OpFalse:
                err = vm_push(vm, OBJ_BOOL(false));
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

            case OpNothing:
                err = vm_push(vm, OBJ_NOTHING);
                if (err) { return err; };
                break;

            case OpSetGlobal:
                // globals index
                pos = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                globals[pos] = vm_pop(vm);
                break;

            case OpGetGlobal:
                // globals index
                pos = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                err = vm_push(vm, globals[pos]);
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
                if (obj.type == o_Error) { return obj.data.err; };

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
                ins = frame_instructions(current_frame);
                break;

            case OpRequire:
                // constants index
                pos = read_big_endian_uint16(ins.data + ip + 1);
                current_frame->ip += 2;

                err = require_sub_module(vm, constants[pos]);
                if (err) { return err; }

                constants = vm->compiler->constants.data; // in case of realloc
                current_frame = vm->frames + vm->frames_index;
                globals = current_frame->function.data.module->globals;
                ins = frame_instructions(current_frame);
                break;

            case OpReturnValue:
                // return value
                obj = vm_pop(vm);

                vm->sp = current_frame->base_pointer;

#ifdef DEBUG
                debug_print_return(current_frame->function, vm->stack + vm->sp,
                                   obj);
#endif

                if (current_frame->function.type == o_Module) {
                    vm->cur_module = vm->cur_module->parent;
                    if (vm->cur_module) {
                        globals = vm->cur_module->globals;
                    } else {
                        globals = vm->globals;
                    }
                }

                current_frame = pop_frame(vm);
                ins = frame_instructions(current_frame);

                err = vm_push(vm, obj);
                if (err) { return err; };
                break;

            case OpReturn:
                vm->sp = current_frame->base_pointer;

#ifdef DEBUG
                debug_print_return(current_frame->function, vm->stack + vm->sp,
                                   OBJ_NOTHING);
#endif

                if (current_frame->function.type == o_Module) {
                    vm->cur_module = vm->cur_module->parent;
                    if (vm->cur_module) {
                        globals = vm->cur_module->globals;
                    } else {
                        globals = vm->globals;
                    }
                }

                current_frame = pop_frame(vm);
                ins = frame_instructions(current_frame);

                err = vm_push(vm, OBJ_NOTHING);
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

                if (constants[pos].type != c_Function) {
                    return errorf("not a function: constant %d", pos);
                }

                err = vm_push_closure(vm, constants[pos], num);
                if (err) { return err; };
                break;

            case OpGetFree:
                // free variable index
                pos = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                err = vm_push(vm, current_frame->function.data.closure->free[pos]);
                if (err) { return err; };
                break;
            case OpSetFree:
                // free variable index
                pos = read_big_endian_uint8(ins.data + ip + 1);
                current_frame->ip += 1;

                current_frame->function.data.closure->free[pos] = vm_pop(vm);
                break;

            case OpCurrentClosure:
                err = vm_push(vm, OBJ(o_Closure, .closure = current_frame->function.data.closure));
                if (err) { return err; };
                break;

            default:
                return errorf("unknown opcode %d", op);
        }
    }
    return 0;
}

Object vm_last_popped(VM *vm) {
    return vm->stack[vm->sp];
}

static void
_print_repeats(int first_idx, int cur_idx) {
    int repeats = cur_idx - first_idx - 1;
    if (repeats > 1) {
        printf("[Previous line repeated %d more times]\n", repeats);
    }
}

static void
frame_init(VM *vm, Object function, int base_pointer) {
    vm->frames[vm->frames_index] = (Frame) {
        .function = function,
        .ip = -1,
        .base_pointer = base_pointer,
    };
}

static Instructions
frame_instructions(Frame *f) {
    switch (f->function.type) {
        case o_Closure:
            return f->function.data.closure->func->instructions;
        case o_Module:
            return f->function.data.module->main_function->instructions;
        default:
            die("frame_instructions: Frame should not have function of type %s",
                show_object_type(f->function.type));
    }
}

static error module_compile(VM *vm, Module *m, const char *source_code) {
    bool success = false;

    // create dummy file that appends writes into [buf], to return as error.
    char *buf = NULL;
    size_t buf_len = 0;
    FILE *fp = open_memstream(&buf, &buf_len);
    if (fp == NULL) {
        return errorf("error loading module - %s", strerror(errno));
    }

    Lexer l;
    lexer_init(&l, source_code, strlen(source_code));

    Program program = {0};
    ParseErrorBuffer errors = parse(parser(), &l, &program);
    if (errors.length > 0) {
        fputs(parser_error_msg, fp);
        print_parse_errors(&errors, fp);
        program_free(&program);
        goto cleanup;
    }

    Compiler *c = vm->compiler;

    // Enter Independent Compilation Scope
    SymbolTable *prev = NULL;
    prev = c->cur_symbol_table;
    c->cur_symbol_table = NULL;
    enter_scope(c);

    error err = compile(c, &program);
    if (err) {
        fputs(compiler_error_msg, fp);
        print_error(err, fp);
        free_error(err);
        compiler_reset(c);
        goto cleanup;
    }

    m->program = program;
    module_bytecode(m, bytecode(c));

    // Exit Independent Compilation Scope
    c->cur_symbol_table->outer = prev;
    SymbolTable *module_table = c->cur_symbol_table;
    leave_scope(c);
    symbol_table_free(module_table);

    success = true;

cleanup:
    fclose(fp);

    if (!success) {
        program_free(&program);
        return create_error(buf);
    }

    free(buf);
    return 0;
}

error require_sub_module(VM *vm, Constant constant) {
    error err;

    if (constant.type != c_String) {
        die("require - filename constant of type %d, not c_String",
            constant.type);
    }

    Token *string = constant.data.string;
    uint64_t hash = hash_fnv1a_(string->start, string->length);
    Module *module = ht_get_hash(vm->sub_modules, hash);
    if (errno == ENOKEY) {
        char *filename = strndup(string->start, string->length);
        if (filename == NULL) { die("require - strndup filename:"); }

        module = calloc(1, sizeof(Module));
        if (module == NULL) { die("require - create Module"); }

        err = load_file(filename, (char **) &module->source);
        if (err) { goto fail; }

        module->name = filename;

        err = module_compile(vm, module, module->source);
        if (err) { goto fail; }

        ht_set_hash(vm->sub_modules, (void *) filename, module, hash);
        if (errno == ENOMEM) {
            err = errorf("error adding module - %s", strerror(errno));
            goto fail;
        }
    }

    module->parent = vm->cur_module;
    vm->cur_module = module;

#ifdef DEBUG
    printf("require module: ");
    Object *args = vm->stack + vm->sp;
    debug_print_args(OBJ(o_Module, .module = module), args, 0);
    putc('\n', stdout);
#endif


    err = new_frame(vm);
    if (err) { return err; }

    frame_init(vm, OBJ(o_Module, .module = module), vm->sp);
    return 0;

fail:
    module_free(module);
    return err;
}

void print_vm_stack_trace(VM *vm, FILE *s) {
    Closure *prev = NULL;
    int prev_ip = -1,
        prev_idx = 0,
        idx = 0;

    SourceMapping *mapping = NULL;
    for (; idx <= vm->frames_index; ++idx) {
        Frame frame = vm->frames[idx];

        if (frame.function.data.closure == prev && frame.ip == prev_ip) {
            // [frame] stopped at the same position as previous.
            continue;

        } else {
            _print_repeats(prev_idx, idx);

            // previous Frame if present, stopped at the same position as
            // current.
            Object func = frame.function;
            switch (func.type) {
                case o_Closure:
                    mapping = find_mapping(&func.data.closure->func->mappings, frame.ip);

                    prev = func.data.closure;
                    prev_idx = idx;
                    prev_ip = frame.ip;
                    break;
                case o_Module:
                    mapping = find_mapping(&func.data.module->main_function->mappings, frame.ip);
                    break;
                default:
                    die("print_vm_stack_trace: Frame should not have function of type %s",
                        show_object_type(func.type));
            }

        }

        object_fprint(frame.function, stdout);

        if (mapping) {
            Token *tok = node_token(mapping->node);
            fprintf(s, ", line %d\n", tok->line);
            highlight_token(tok, /* leftpad */ 2, s);
        } else {
            putc('\n', s);
        }
    }

    _print_repeats(prev_idx, idx);
}
