#include "vm.h"
#include "code.h"
#include "compiler.h"
#include "object.h"
#include "utils.h"

#define TRUE OBJ(o_Boolean, .boolean = true)
#define FALSE OBJ(o_Boolean, .boolean = false)

void vm_init(VM *vm, Bytecode* code) {
    vm->constants = code->constants;
    vm->instructions = code->instructions;
    vm->errors = (StringBuffer){};
    vm->stack = calloc(StackSize, sizeof(Object));
    if (vm->stack == NULL) { die("calloc stack"); };
    vm->sp = 0;
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
execute_binary_integer_operation(
        VM *vm, Opcode op, Object left, Object right) {
    long left_value = left.data.integer;
    long right_value = right.data.integer;
    long result;

    switch (op) {
        case OpAdd:
            result = left_value + right_value;
            break;
        case OpSub:
            result = left_value - right_value;
            break;
        case OpMul:
            result = left_value * right_value;
            break;
        case OpDiv:
            result = left_value / right_value;
            break;
        default:
            error(&vm->errors, "unkown integer operator: (Opcode: %d)", op);
            return -1;
    }

    return vm_push(vm, OBJ(o_Integer, .integer = result));
}

static int
execute_binary_operation(VM *vm, Opcode op) {
    Object right = vm_pop(vm);
    Object left = vm_pop(vm);

    if (left.typ == o_Integer && right.typ == o_Integer) {
        return execute_binary_integer_operation(vm, op, left, right);
    }
    error(&vm->errors,
            "unsupported types for binary operation: %s (Opcode: %d) %s",
            show_object_type(left.typ), op, show_object_type(right.typ));
    return -1;
}

static Object
native_bool_to_boolean_object(bool input) {
    return OBJ(o_Boolean, .boolean = input);
}

static int
execute_integer_comparison(VM *vm, Opcode op, Object left, Object right) {
    long left_value = left.data.integer;
    long right_value = right.data.integer;
    Object result;

    switch (op) {
        case OpEqual:
            result = native_bool_to_boolean_object(left_value == right_value);
            break;
        case OpNotEqual:
            result = native_bool_to_boolean_object(left_value != right_value);
            break;
        case OpGreaterThan:
            result = native_bool_to_boolean_object(left_value > right_value);
            break;
        default:
            error(&vm->errors, "unkown operator: (Opcode: %d)", op);
            return -1;
    }
    return vm_push(vm, result);
}

static int
execute_comparison(VM *vm, Opcode op) {
    Object right = vm_pop(vm);
    Object left = vm_pop(vm);

    if (left.typ == o_Integer && right.typ == o_Integer) {
        return execute_integer_comparison(vm, op, left, right);
    }

    bool eq = object_eq(&left, &right);

    switch (op) {
        case OpEqual:
            return vm_push(vm, native_bool_to_boolean_object(eq));
        case OpNotEqual:
            return vm_push(vm, native_bool_to_boolean_object(!eq));
        default:
            error(&vm->errors, "unkown operator: (Opcode: %d) (%s %s)",
                    op, show_object_type(left.typ), show_object_type(right.typ));
            return -1;
    }
}

static int
execute_bang_operator(VM *vm) {
    Object operand = vm_pop(vm);
    switch (operand.typ) {
        case o_Boolean:
            operand.data.boolean = !operand.data.boolean;
            return vm_push(vm, operand);
        case o_Null:
            return vm_push(vm, TRUE);
        default:
            return vm_push(vm, FALSE);
    }
}

static int
execute_minus_operator(VM *vm) {
    Object operand = vm_pop(vm);
    if (operand.typ != o_Integer) {
        error(&vm->errors, "unsupported type for negation: %s",
                show_object_type(operand.typ));
        return -1;
    }
    operand.data.integer = -operand.data.integer;
    return vm_push(vm, operand);
}

static bool
is_truthy(Object obj) {
    switch (obj.typ) {
        case o_Boolean:
            return obj.data.boolean;
        case o_Null:
            return false;
        default:
            return true;
    }
}

int vm_run(VM *vm) {
    Opcode op;
    int err, const_index;

    for (int ip = 0; ip < vm->instructions.length; ip++) {
        op = vm->instructions.data[ip];
        switch (op) {
            case OpConstant:
                const_index =
                    read_big_endian_uint16(vm->instructions.data + ip + 1);
                ip += 2;

                err = vm_push(vm, vm->constants.data[const_index]);
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
                err = vm_push(vm, TRUE);
                if (err == -1) { return -1; };
                break;
            case OpFalse:
                err = vm_push(vm, FALSE);
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
                {
                    int pos = read_big_endian_uint16(vm->instructions.data + ip + 1);
                    ip = pos - 1;
                    break;
                }
            case OpJumpNotTruthy:
                {
                    int pos = read_big_endian_uint16(vm->instructions.data + ip + 1);
                    ip += 2;

                    Object condition = vm_pop(vm);
                    if (!is_truthy(condition)) {
                        ip = pos - 1;
                    }
                    break;
                }

            case OpNull:
                err = vm_push(vm, (Object){});
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

void vm_destroy(VM *vm) {
    // constants and instructions are freed by [Compiler]
    for (int i = 0; i < vm->errors.length; i++) {
        free(vm->errors.data[i]);
    }
    // invalid pointer.. for some reason.
    free(vm->stack);
}
